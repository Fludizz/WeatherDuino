#!/usr/bin/python2.7
# -*- coding: utf8 -*-
""" UDP listening client for the Weatherduino UDP output"""
__author__ = 'Jan KLopper (jan@underdark.nl)'
__version__ = 0.1

PROTOCOLVERSION = (1,2)
MAGIC = 101

import socket
import argparse
import struct
import time
import os
import sys
import pickle
import ConfigParser
import math

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
    if magic == MAGIC and version in PROTOCOLVERSION:
      device = (ord(data[2]), ord(data[3]), ord(data[4]))
      if self.options.filter and self.options.filter != device:
        if self.verbose:
          print 'skipping device: %r' % device
      else:
        probecount = ord(data[5])
        offset = 6
        if self.verbose:
          print '%d probes found on %x:%x:%x' % (
              probecount, device[0], device[1], device[2])
        if version == 1:
          fragmentsize = 3
          for sensor in range(0, probecount):
            sensoroffet = (sensor*fragmentsize)+offset
            humidity = ord(data[sensoroffet+2])
            temp = (ord(data[sensoroffet]),
                    ord(data[sensoroffet+1]))
            yield (device, sensor, temp, humidity)
        elif version == 2:
          fragmentsize = 5
          for sensor in xrange(0, probecount):
            sensoroffet = (sensor*fragmentsize)+offset
            humidity = ord(data[sensoroffet+4])
            rawtemp = data[sensoroffet:sensoroffet+4]
            temp = struct.unpack('<f', rawtemp)[0]
            temp = (int(math.floor(temp)), int((temp - math.floor(temp))*100))
            yield (device, sensor, temp, humidity)

  def PrintMeasurements(self, device, sensor, temp, humidity):
    """Prints the data for a sensor if either temperature or humidty is valid"""
    device = '%x:%x:%x' % (device[0], device[1], device[2])
    try:
      sensor = self.options.config.get(device, str(sensor))
    except (ConfigParser.NoOptionError, ConfigParser.NoSectionError):
      pass
    print device
    if temp[0] < 129 or humidity < 255:
      print 'Sensor %s:' % sensor
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
          CREATE TABLE sensors (date, device, sensor, temp, humidity)""")
        self.logconnection.commit()

    def StoreMeasurements(self, device, sensor, temp, humidity):
      """Store the data for a sensor if either temperature or humidity is valid"""
      cursor = self.logconnection.cursor()
      if temp[0] < 129 or humidity < 255:
        cursor.execute("INSERT INTO sensors VALUES (%d,'%s',%d,%f,%d)" % (
            int(time.time()),
            device,
            sensor,
            float('%d.%02d' % temp),
            humidity))
        self.logconnection.commit()


class CarbonWeatherDuinoListener(WeatherDuinoListener):
  """This abstraction saves the collected data to a Carbon server"""
  def __init__(self, options, handler):
    super(CarbonWeatherDuinoListener, self).__init__(options, handler)
    try:
      server, port = options.carbon.split(':')
      self.carbonsocket = socket.create_connection((server, int(port)))
    except socket.error, msg:
      print 'Cannot connect to carbon server: %r' % msg
      sys.exit()

  def StoreMeasurements(self, device, sensor, temp, humidity):
    """Store the data for a sensor if either temperature or humidty is valid"""
    output = {}
    device = '%x:%x:%x' % (device[0], device[1], device[2])
    if temp[0] < 129 or humidity < 255:
      try:
        sensor = self.options.config.get(device, str(sensor))
      except (ConfigParser.NoOptionError, ConfigParser.NoSectionError):
        pass
      path = 'weather.%s' % (device, sensor)
    if temp[0] < 129:
      output['%s.temp' % path] = float('%d.%02d' % temp)
    if humidity < 255:
      output['%s.humidity' % path] = humidity
    self.SendToCarbon(output)

  def SendToCarbon(self, output):
    """Wrapper to output data to Carbon as Pickeld data"""
    timestamp = int(time.time())
    items = []
    for key, stat in sorted(output.items()):
      items.append( (key, (timestamp, stat)))
    payload = pickle.dumps(items)
    header = struct.pack("!L", len(payload))
    message = header + payload
    self.carbonsocket.send(message)

  def __del__(self):
    self.carbonsocket.shutdown(1)
    self.carbonsocket.close()

def main():
  """This program listenes to the broadcast address on the listening port and
  handles any received measurements

  Measurements are stored in:
	 sqlite (if available), logfile, Carbon, or outputs to stdout"""
  parser = argparse.ArgumentParser()
  parser.add_argument("-p", "--port", dest="port",
                    help="Listen port", default=65001)
  parser.add_argument("-v", "--verbose", dest="verbose",
                    action="store_true",
                    help="Be verbose", default=False)
  parser.add_argument("-l", "--logoutput", dest="log",
                    help="Output file")
  parser.add_argument("-c", "--carbon", dest="carbon",
                    help="Carbon server")
  parser.add_argument("-f", "--filter", dest="filter",
                    help="Filter device")
  if sqlite:
    parser.add_argument("-s", "--sqloutput", dest="sql",
                      help="sqlite file")
  options = parser.parse_args()

  config = ConfigParser.ConfigParser()
  try:
    config.read(['.weatherduino', os.path.expanduser('~/.weatherduino')])
    options.config = config
  except:
    pass

  if options.log:
    wduino = LogWeatherDuinoListener(options, 'StoreMeasurements')
  elif options.carbon:
    wduino = CarbonWeatherDuinoListener(options, 'StoreMeasurements')
  elif options.sql and sqlite:
    wduino = SQLWeatherDuinoListener(options, 'StoreMeasurements')
  else:
    wduino = WeatherDuinoListener(options, 'PrintMeasurements')
  wduino.Run()

if __name__ == '__main__':
  main()
