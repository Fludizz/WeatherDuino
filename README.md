WeatherDuino
============

A simple weather station based on an Arduino, DHT11 and DS18B20.
See http://frack.nl/wiki/WeatherDuino (Dutch) for more details on the project.


DHT11 Library
=============

Get it here: https://github.com/markruys/arduino-DHT
This project uses a autosensing DHT11/DHT22/AM2302/RHT03 library

OneWire Library
===============

Get it here: http://www.pjrc.com/teensy/td_libs_OneWire.html
This OneWire library can be used without modifications.

DallasTemperature Library
=========================

Get it here: https://github.com/milesburton/Arduino-Temperature-Control-Library
This DallasTemperature library can be used without modifications.

Auto detection
==============
The WeatherDuino automatically detects any onewire sensors you might have.
It does this using the following procedure:
* An empty Weatherduino (fresh from the programmer) boots
* Finds it has no onewire addresses stored
* Starts scanning the onewire bus
* Finds the first (onboard device) and stores it's address in eeprom
* waits for the 2nd sensorsboard to be connected
* Stores its address
* it repeats these steps untill it finds four sensors, or untill you reset the
  device
* After reset the device will read the number of detected sensors from the
  EEPROM and use those, to restart scanning disconnect a sensor.

Serial Output
=============

Serial Output is JSON, the output looks like this:

{ "WeatherDuino": 
  [ 
    { "probe": 1, "temp": 23.12, "humid", 45.00}, 
    { "probe": 2, "temp": 15.24, "humid": 73.00 }, 
    { "probe": 3, "temp": -10.20, "humid": 20.00 },
    { "probe": 2, "temp": 0.24, "humid": 39.00 }
  ] 
}

All on a single line.

UDP output
==========

The arduino can output udp over an ecn2860 ic, for this you need th ethercard 
library available here: https://github.com/jcw/ethercard

The udp packet will hold:
first byte: number of probes
each subsequent set of 3 bytes, for each probe:
temp, temp fractions, humidity

For the ENC28J60 module, connect the following (these are etherCard defaults):
Or place the ecn on the weatherduino board as shown on the silkscreen.
* VCC -> 3.3V
* GND -> GND
* SCK (clock)    -> pin 13 (Hardware SPI clock)
* SO (slave out) -> pin 12 (Hardware SPI master-in)
* SI (slave in)  -> pin 11 (Hardware SPI master-out)
* CS (chip-select) -> pin 8

Tools
=====
The udplistener in the tools directory allows you to listen to a networked 
weatherduino to pick up its signals and store/send them in/to:
* A text file
* A sqlite database
* A Graphite / Carbon backend
* Stdout
