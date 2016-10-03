# Dashboard Demo

This is the code for the Carloop demo at World Maker Faire 2016: a
dashboard from a car that is controlled through Carloop.

The main program reads 3 knobs (engine speed, vehicle speed and engine
temperature) and transmit CAN messages with those values to control the
dials in the the dashboard.

The Carloop also interfaces a Linux laptop or Raspberry Pi running
`canutils`, a suite of program for car hacking.
