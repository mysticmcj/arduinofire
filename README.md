# Arduinofire #
### a simple environmental/fire/gas sensor. ###

* Please be nice, this is literally my first Ardunio/Teensy project! *

This is the beginning of something that I am going to use to monitor
shared workshop space that consists of kilns, torches, forges, and
other dangerous things.

All of the hardware was bargain-bin DIYMall/Kookye junk purchased
cheaply on Amazon/ebay - there are a number of "brands" that these are
sold under, from the reputable like Adafruit, to the other end of
the spectrum, where names can change daily.

Hardware requirements:
- Arduino compatible microcontroller - I've tested on a teensy 3.1
- Adafruit/DIYMall/whatever 128x64 mono OLEd display
- Fire sensor
- MQ5 gas sensor
- Temp/Hum sensor
- Barometer (optional)
- Piezo "sensor" (this is a three wire module - vcc, gnd, signal)
- red/green LEDs or whatever suits your fancy
- 330k or whatever resistors that are appropriate for your LEDs
- Relay board, ideally one that works, and possibly with an isolated power supply.

Specifics of the relays are out of scope of this document, and are the part that I cannot get working at present.
It appears that the kookye relay board that I'm using is at fault, as I cannot get the relays to fire even with their own power supply
using optocoupled signaling.  The LEDs next to them happily come on as appropriate for an normally open / normally closed circuit, so
the logic is sane.

Schematics are not included as literally everything is direct to sensors with a common VCC/ground, with the exception of the LEDs.

Don't go cheap like I did for this project - Support Arduino, Adafruit, and Teensy (PJRC) direct whenever possible, or other vendors that
contribute back to the community.

