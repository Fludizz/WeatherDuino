#!/usr/bin/python2.7
# -*- coding: utf8 -*-
""" UDP listening client for the Weatherduino UDP output"""
__author__ = 'Jan KLopper (jan@underdark.nl)'
__version__ = 0.1

import socket
import optparse
import struct

def WeatherDuinoListener(port, handler):
  """This function listens for udp packets on all ethernet interfaces on the 
  given port and processes them."""
  UDPSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  UDPSock.bind(('', port))

  while True:
    data, addr = UDPSock.recvfrom(1024)
    for measurement in ProcessPacket(data):
      handler(*measurement)
  UDPSock.close()

def ProcessPacket(data):
  """This processes the actual data packet"""
  device = (ord(data[0]), ord(data[1]), ord(data[2]))
  probecount = ord(data[3])
  print '%d probes found on %x:%x:%x' % (
      probecount, device[0], device[1], device[2])
  for sensor in range(0, probecount):
    humidity = ord(data[(sensor*3)+6:(sensor*3)+7])
    temp = (ord(data[(sensor*3)+4:(sensor*3)+5]), 
            ord(data[(sensor*3)+5:(sensor*3)+6]))
    yield (device, sensor, temp, humidity)

def PrintMeasurements(device, sensor, temp, humidity):
  """Prints the data for a sensor if either temperature or humidty is valid"""
  if temp[0] < 129 or humidity < 255:
    print 'Sensor %i:' % sensor
  if temp[0] < 129:
    print '\ttemp: %d.%02d°' % temp 
  if humidity < 255:
    print '\thumidity: %d%%' % humidity
  
def main():
  """This program listenes to the broadcast address on the listening port and 
  outputs any received measurements"""
  parser = optparse.OptionParser()
  parser.add_option("-p", "--port", dest="port",
                    help="Listen port", default=65001)
  (options, args) = parser.parse_args()

  WeatherDuinoListener(options.port, PrintMeasurements)

if __name__ == '__main__':
  main()
