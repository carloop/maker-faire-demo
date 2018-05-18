/* Transmit the value of some knobs connected to the Carloop as CAN messages
 */

#include "application.h"
#include "carloop.h"
#include "socketcan_serial.h"

SYSTEM_THREAD(ENABLED);

void setupKnobs();
void readKnobs();
void readKnob(unsigned i);
void normalizeKnob(unsigned i);
void printKnobs();
void transmitCAN();

Carloop<CarloopRevision2> carloop;

/* Connect potentiometer of each knob to powerPin, groundPin and the
 * appropriate knobPin.
 * Run the program and adjust knobValueLow and knobValueHigh to get 100%
 * when the knob is at each end stop. If values are reversed (100% for
 * low stop), reserve powerPin and groundPin.
 */
const unsigned knobCount = 3;
const int powerPin = A2;
const int groundPin = A3;
const int knobPin[knobCount] = { A4, A5, A6 };
uint16_t knobValueRaw[knobCount] = { 0 }; // 3.3V = 4096
uint16_t knobValueLow[knobCount] = { 30, 30, 30 };
uint16_t knobValueHigh[knobCount] = { 4060, 4060, 4060 };
uint16_t knobPercent[knobCount] = { 0 }; // 100% = 32768
const uint16_t knob100Percent = 32768;
uint8_t pedalPosition = 0;
const uint8_t pedalPositionMin = 0x28;

/* every
 * Helper than runs a function at a regular millisecond interval
 */
template <typename Fn>
void every(unsigned long intervalMillis, Fn fn) {
  static unsigned long last = 0;
  if (millis() - last > intervalMillis) {
    last = millis();
    fn();
  }
}

void setup() {
  Serial.begin(9600);
  setupKnobs();
  carloop.begin();
}

void setupKnobs() {
  RGB.control(true);
  pinMode(powerPin, OUTPUT);
  digitalWrite(powerPin, HIGH);

  pinMode(groundPin, OUTPUT);
  digitalWrite(groundPin, LOW);

  for (unsigned i = 0; i < knobCount; i++) {
    pinMode(knobPin[i], INPUT);
  }
}

void loop() {
  socketcanReceiveMessages();
  readKnobs();
  printKnobs();
  transmitCAN();
}

void readKnobs() {
  for (unsigned i = 0; i < knobCount; i++) {
    readKnob(i);
    normalizeKnob(i);
  }
}

void readKnob(unsigned i) {
  knobValueRaw[i] = analogRead(knobPin[i]);
}

/* normalizeKnob
 * Scale the raw ADC count between the low and high value, normalize
 * to a percentage value and limit between 0% and 100%.
 */
void normalizeKnob(unsigned i) {
  uint16_t range = knobValueHigh[i] - knobValueLow[i];
  int32_t percent = (int32_t)(knobValueRaw[i] - knobValueLow[i]) * knob100Percent / range;
  knobPercent[i] = (percent < 0) ? 0 : (percent > knob100Percent) ? knob100Percent : percent;
}

uint8_t percentToByte(uint16_t percent) {
  if (percent >= knob100Percent) {
    return 255;
  } else {
    return percent / (knob100Percent / 256);
  }
}

void printKnobs() {
  every(200, [] {
    RGB.color(
      percentToByte(knobPercent[0]),
      percentToByte(knobPercent[1]),
      percentToByte(knobPercent[2])
    );
  });
}

/* transmitCAN
 * Send CAN messages with the values of the knobs at regular intervals
 * Put multiple every(interval, ...) statements to send multiple CAN
 * messages at different intervals
 */
CANMessage engineSpeedMessage;
CANMessage vehicleSpeedMessage;
CANMessage engineTemperatureMessage;
void transmitCAN() {
  every(2, [] {
    engineSpeedMessage.id = 0x0C9;
    engineSpeedMessage.len = 8;
    if (pedalPosition > 0) {
      engineSpeedMessage.data[1] = pedalPosition;
    } else {
      engineSpeedMessage.data[1] = knobPercent[0] / 256; // 128 = 8000 rpm
    }
    carloop.can().transmit(engineSpeedMessage);
    printReceivedMessage(engineSpeedMessage);

    vehicleSpeedMessage.id = 0x3E9;
    vehicleSpeedMessage.len = 8;
    uint16_t vehicleSpeed = knobPercent[1] / 2;
    vehicleSpeedMessage.data[0] = (uint8_t) (vehicleSpeed >> 8);
    vehicleSpeedMessage.data[1] = (uint8_t) (vehicleSpeed & 0xFF);
    carloop.can().transmit(vehicleSpeedMessage);
    printReceivedMessage(vehicleSpeedMessage);

    engineTemperatureMessage.id = 0x4C1;
    engineTemperatureMessage.len = 8;
    const uint8_t tmin = 0x57;
    const uint8_t tmax = 0xAA;
    uint8_t engineTemperature = (uint8_t)((uint32_t)knobPercent[2] * (tmax - tmin) / knob100Percent + tmin);
    engineTemperatureMessage.data[2] = engineTemperature;
    carloop.can().transmit(engineTemperatureMessage);
    printReceivedMessage(engineTemperatureMessage);
  });

  every(10, [] {
    CANMessage message;

    message.id = 0x123;
    message.len = 3;
    message.data[0] = percentToByte(knobPercent[0]);
    message.data[1] = percentToByte(knobPercent[1]);
    message.data[2] = percentToByte(knobPercent[2]);

    carloop.can().transmit(message);
    printReceivedMessage(message);
  });

  every(100, [] {
    // Request pedal position
    CANMessage message;

    message.id = 0x7E0;
    message.len = 3;
    message.data[0] = 0x02; // 2 bytes
    message.data[1] = 0x01; // OBD-II Read PID
    message.data[2] = 0x49; // Accelerator pedal position

    carloop.can().transmit(message);
    printReceivedMessage(message);
  });
}

void applicationCanReceiver(CANMessage &message) {
  if (message.id == 0x0C9) {
    engineSpeedMessage.data[0] = message.data[0];
    engineSpeedMessage.data[1] = message.data[1];
    engineSpeedMessage.data[2] = message.data[2];
    engineSpeedMessage.data[3] = message.data[3];
    engineSpeedMessage.data[4] = message.data[4];
    engineSpeedMessage.data[5] = message.data[5];
    engineSpeedMessage.data[6] = message.data[6];
    engineSpeedMessage.data[7] = message.data[7];
  }

  if (message.id == 0x7E8) {
    uint8_t pos = message.data[3];
    pedalPosition = pos > pedalPositionMin ? pos - pedalPositionMin : 0;
  }
}
