# Dashboard Demo

This is the code for the Carloop demo at World Maker Faire 2016 in New
York: a dashboard from a car that is controlled through Carloop.

The main program reads 3 knobs (engine speed, vehicle speed and engine
temperature) and transmit CAN messages with those values to control the
dials in the the dashboard.

The Carloop also interfaces a Linux laptop or Raspberry Pi running
`canutils`, a suite of program for car hacking.

## Hardware

The 3 knobs must be connect this way:
- Connect the top pin of each knob to common power.
- Connect the bottom pin of each knob to common ground.
- Connect the middle pin of each pin to different signal wires

- A2: common power
- A3: common ground
- A4: engine speed signal
- A5: vehicle speed signal
- A6: engine temperature signal

## Instructions

Compile and flash the firmware using the Particle CLI

```
carloop/dashboard_demo$ particle flash <carloop_device_name> Firmware
```

