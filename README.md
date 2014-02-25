WeatherDuino
============

A simple weather station based on an Arduino, DHT11 and DS18B20.
See http://frack.nl/wiki/WeatherDuino (Dutch) for more details on the project.


DHT11 Library
=============

Get it here: https://github.com/adafruit/DHT-sensor-library
This project uses the generic DHT11 library, however for the JSON output to work
on the serial line, the DHT.cpp file must be editted to remove the lines 
containing "Serial.print()"

OneWire Library
===============

Get it here: http://www.pjrc.com/teensy/td_libs_OneWire.html
This OneWire library can be used without modifications.

DallasTemperature Library
=========================

Get it here: https://github.com/milesburton/Arduino-Temperature-Control-Library
This DallasTemperature library can be used without modifications.


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
=============

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


