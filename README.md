WeatherDuino
============

A simple weather station based on an Arduino, DHT11 and DS18B20.

See http://frack.nl/wiki/WeatherDuino (Dutch) for more details on the project.

Components
==========
* Arduino
* 2x DS18B20
* 1x 4.7k Ohm resistor (Pull-up OneWire bus)
* 2x DHT11
* 2x 10k Ohm resistor (Pull-up DHT Data bus)
* 16x4 HD44780 compatible display
* 1.8k Ohm resistor (Contrast for LCD)
* 2x RJ45 socket

DHT11 Library
=============

This project uses the generic DHT11 library, however for the JSON output to work
on the serial line, the DHT.cpp file must be editted to remove the lines 
containing "Serial.print()"

Serial Output
=============

Serial Output is JSON, the output looks like this:
{ "WeatherDuino": [ { "probe": 1, "temp": 23, "humid", 45}, { "probe": 2, "temp": 15, "humid": 74 } ] }
