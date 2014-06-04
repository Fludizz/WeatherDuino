#!/usr/bin/python2.7
# -*- coding: utf8 -*-
""" UDP listening client for the Weatherduino UDP output"""
__author__ = 'Jan KLopper (jan@underdark.nl)'
__version__ = 0.1

PROTOCOLVERSION = 1
MAGIC = 101

import socket
import argparse
import struct
import time
import os

try:
  import sqlite3
  sqlite = True
except ImportError:
  sqlite = False

class WeatherDuinoListener(object):
  def __init__(self, options, handler):
    """This function listens for udp packets on all ethernet interfaces on the 
    given port and processes them."""

    self.options = options
    self.UDPSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    self.UDPSock.bind(('', options.port))
    self.verbose = options.verbose
    self.handler = getattr(self, handler)
    print 'Starting WeatherDuino listener'

  def Run(self):
    count = 0
    while True:      
      data, addr = self.UDPSock.recvfrom(1024)
      if self.verbose:
        count = count + 1
        print 'Packet %s @ %s' % (count, time.strftime("%H:%M:%S"))
      for measurement in self.ParsePacket(data):
        self.handler(*measurement)

  def ParsePacket(self, data):
    """This processes the actual data packet"""
    magic = ord(data[0])
    version = ord(data[1])
    if magic == MAGIC and version == PROTOCOLVERSION:
      device = (ord(data[2]), ord(data[3]), ord(data[4]))
      probecount = ord(data[5])
      offset = 6
      if self.verbose:
        print '%d probes found on %x:%x:%x' % (
            probecount, device[0], device[1], device[2])
      for sensor in range(0, probecount):
        humidity = ord(data[(sensor*3)+offset+2:(sensor*3)+offset+3])
        temp = (ord(data[(sensor*3)+offset:(sensor*3)+offset+1]), 
                ord(data[(sensor*3)+offset+1:(sensor*3)+offset+2]))
        yield (device, sensor, temp, humidity)

  def PrintMeasurements(self, device, sensor, temp, humidity):
    """Prints the data for a sensor if either temperature or humidty is valid"""
    if temp[0] < 129 or humidity < 255:
      print 'Sensor %i:' % sensor
    if temp[0] < 129:
      print '\ttemp: %d.%02d°' % temp 
    if humidity < 255:
      print '\thumidity: %d%%' % humidity

  def __del__(self):
    self.UDPSock.close()

class LogWeatherDuinoListener(WeatherDuinoListener):
  """This abstraction saves the collected data to a logfile"""
  def __init__(self, options, handler):
    self.logfile = file(options.log, 'a')
    super(LogWeatherDuinoListener, self).__init__(options, handler)

  def StoreMeasurements(self, device, sensor, temp, humidity):
    """Store the data for a sensor if either temperature or humidty is valid"""
    self.logfile.write('\nDevice: %x:%x:%x\t' % (
        device[0], device[1], device[2]))
    if temp[0] < 129 or humidity < 255:
      self.logfile.write('%s\tSensor %i:' % (time.strftime("%H:%M:%S"), sensor))
    if temp[0] < 129:
      self.logfile.write('\ttemp: %d.%02d° - ' % temp)
    if humidity < 255:
      self.logfile.write('\thumidity: %d%%' % humidity)

if sqlite:
  class SQLWeatherDuinoListener(WeatherDuinoListener):
    """This abstraction saves the collected data to a sqllite backend"""
    
    def __init__(self, options, handler):
      super(SQLWeatherDuinoListener, self).__init__(options, handler)
      skipcreate = False
      if os.path.exists(options.sql):
        skipcreate = True
      self.logconnection = sqlite3.connect(options.sql)
      if not skipcreate:
        if options.verbose:
          print 'Create sqlite database %s' % options.sql
        cursor = self.logconnection.cursor()
        cursor.execute("""
          CREATE TABLE sensors (date, device, sensor, temp, humidty)""")
        self.logconnection.commit()
      
    def StoreMeasurements(self, device, sensor, temp, humidity):
      """Store the data for a sensor if either temperature or humidty is valid"""
      cursor = self.logconnection.cursor()
      
      if temp[0] < 129 or humidity < 255:
        cursor.execute("INSERT INTO sensors VALUES ('%s','%s',%d,%f,%d)" % (
            time.strftime("%H:%M:%S"),
            device,
            sensor,
            float('%d.%02d' % temp), 
            humidity))
        self.logconnection.commit()

def main():
  """This program listenes to the broadcast address on the listening port and 
  handles any received measurements
  
  Measurements are stored in sql (if available), logfile, or output to stdout"""
  parser = argparse.ArgumentParser()
  parser.add_argument("-p", "--port", dest="port",
                    help="Listen port", default=65001)
  parser.add_argument("-v", "--verbose", dest="verbose",
                    action="store_true",
                    help="Be verbose", default=False)                    
  parser.add_argument("-l", "--logoutput", dest="log",
                    help="Output file")
  if sqlite:
    parser.add_argument("-s", "--sqloutput", dest="sql",
                      help="sqlite file")
  options = parser.parse_args()

  if options.log:
    wduino = LogWeatherDuinoListener(options, 'StoreMeasurements')  
  elif options.sql and sqlite:
    wduino = SQLWeatherDuinoListener(options, 'StoreMeasurements')  
  else:
    wduino = WeatherDuinoListener(options, 'PrintMeasurements')
  wduino.Run()

if __name__ == '__main__':
  main()
