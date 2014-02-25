#!/usr/bin/python2.7
""" """
__author__ = 'Jan KLopper (jan@underdark.nl)'
__version__ = 0.1

import socket
import optparse
import struct

def WeatherDuinoListener(port):
  """This function listens for udp packets on all ethernet interfaces on the 
  given port and processes them."""
  UDPSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  UDPSock.bind(('', port))

  while True:
    data, addr = UDPSock.recvfrom(1024)
    ProcessPacket(data)    
  UDPSock.close()

def ProcessPacket(data):
  probecount = ord(data[0])
  print '%d probes found' % probecount
  for sensor in range(0, probecount):
    humidity = ord(data[(sensor*3)+3:(sensor*3)+4])
    temp = (ord(data[(sensor*3)+1:(sensor*3)+2]), ord(data[(sensor*3)+2:(sensor*3)+3]))
    print 'Sensor %i:' % sensor
    if temp[0] < 129:
      print 'temp: %d.%dc' % temp 
    if humidity < 255:
      print 'humidity: %d%%' % ord(data[(sensor*3)+3:(sensor*3)+4])
  
def main():
  parser = optparse.OptionParser()
  parser.add_option("-p", "--port", dest="port",
                    help="Listen port", default=65001)
  (options, args) = parser.parse_args()

  WeatherDuinoListener(options.port)

if __name__ == '__main__':
  main()
