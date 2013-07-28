#!/usr/bin/python2.7
""" RRD Graphing utility for use with the WeatherDuino as found on GitHub:
https://github.com/Fludizz/WeatherDuino
It parses the serial output and writes it to an RRD database file and generates
the new graphs.

More information on how to build this device:
http://frack.nl/wiki/WeatherDuino """
_author__ = 'Rudi Daemen <fludizz@gmail.com>'
__version__ = '0.1'

import rrdtool
import sys
import optparse
import os
import serial
try:
    import json
except ImportError:
    import simplejson as json


# MRTG uses:
# 1 week graphs 30m averages: 6 steps / 800 rows
# 1 Month graphs with 2h avarages: 24 steps / 800 rows
# 1 year graphs with 1 day avarage: 288 steps / 800 rows
# Extracted from mrtg generated RRD file using 'rrdtool info <rrdfile>'.
# Assuming MRTG knows what it's doing, we assume the same values.
DEFRRD = [ "DS:In:GAUGE:600:-100:100",
           "DS:Out:GAUGE:600:-100:100",
           "RRA:AVERAGE:0.5:1:800",
           "RRA:AVERAGE:0.5:6:800",
           "RRA:AVERAGE:0.5:24:800",
           "RRA:AVERAGE:0.5:288:800",
           "RRA:MIN:0.5:6:800",
           "RRA:MIN:0.5:24:800",
           "RRA:MIN:0.5:288:800",
           "RRA:MAX:0.5:6:800",
           "RRA:MAX:0.5:24:800",
           "RRA:MAX:0.5:288:800"]
# Timedelta's in seconds from Now. Required for generating graphs from
# x time ago until Now. Below are the defaults MRTG assumes.
TIMEDELTA = {"day": "-86400", 
             "week": "-604800", 
             "month": "-2628000", 
             "year": "-31536000" }


def ProcessRRDdata(path, rrd, name, unit, inval, outval, defs=DEFRRD):
  ''' This function checks (and optionally creates) the required rrd file and
  will update this file with the data provided. After updating the file, it will
  generate the required graphs. '''
  if path:
    rrdfile = "%s/%s" % (path, rrd)
    imgname = "%s/%s" % (path, name)
  else:
    rrdfile = rrd
    imgname = name
  # Check if the rrdfile exists and create if needed.
  if not os.path.isfile(rrdfile):
    rrdtool.create(rrdfile, defs)
  # update the data in the rrdfile:
  rrdtool.update(rrdfile, "N:%s:%s" % (inval, outval))
  # Generate graphs for each configured Delta in TIMEDELTA:
  for delta in TIMEDELTA.keys():
    if delta == "day":
      rrdtool.graph("%s-%s.png" % (imgname, delta), 
                    "--start", TIMEDELTA[delta],
                    "--vertical-label=%s (%s)" % (name, unit),
                    "--slope-mode",
                    "DEF:indoor=%s:In:AVERAGE" % rrdfile,
                    "DEF:outdoor=%s:Out:AVERAGE" % rrdfile,
                    "LINE1:indoor#00FF00:Indoor\:",
                    "GPRINT:indoor:AVERAGE:%6.1lf" + unit,
                    "LINE1:outdoor#0000FF:Outdoor\:",
                    "GPRINT:outdoor:AVERAGE:%6.1lf" + unit + "\\r")
    else:
      rrdtool.graph("%s-%s.png" % (imgname, delta), 
                "--start", TIMEDELTA[delta],
                "--vertical-label=%s (%s)" % (name, unit),
                "--slope-mode",
                "DEF:inavg=%s:In:AVERAGE" % rrdfile,
                "DEF:outavg=%s:Out:AVERAGE" % rrdfile,
                "DEF:inmin=%s:In:MIN" % rrdfile,
                "DEF:outmin=%s:Out:MIN" % rrdfile,
                "DEF:inmax=%s:In:MAX" % rrdfile,
                "DEF:outmax=%s:Out:MAX" % rrdfile,
                "LINE1:inmax#FFFF00:In Max:",
                "GPRINT:inmax:MAX:%6.1lf" + unit,
                "LINE1:outmax#00FFFF:Out Max:",
                "GPRINT:outmax:MAX:%6.1lf" + unit + "\\r",
                "LINE1:inavg#00FF00:In Avg:",
                "GPRINT:inavg:AVERAGE:%6.1lf" + unit,
                "LINE1:outavg#0000FF:Out Avg:",
                "GPRINT:outavg:AVERAGE:%6.1lf" + unit + "\\r",
                "LINE1:inmin#AAFF00:In Min:",
                "GPRINT:inmin:MIN:%6.1lf" + unit,
                "LINE1:outmin#00AAFF:Out Min:",
                "GPRINT:outmin:MIN:%6.1lf" + unit + "\\r")


def GetWeatherDevice(device="/dev/ttyUSB0", baud=57600):
  ''' Generate a device/object useable for the program, if it is a valid device
  (e.g. a WeatherDuino). '''
  weatherduino = serial.Serial(device, baud, timeout=10)
  # Clear the initial junk from the buffer
  weatherduino.readline()
  if "WeatherDuino" in json.loads(weatherduino.readline()):
    return weatherduino
  else:
    raise Exception('Device is not a WeatherDuino!')


def ContinualRRDwrite(arduino, destination):
  ''' With the returned WeatherDuino device, run an endless loop collecting and
  writing weather data. '''
  if not os.path.exists(destination):
    raise Exception('Destination path does not exist.')
  while True:
    arduino.flushInput()
    weatherduino.readline()
    data = json.loads(arduino.readline().strip())['WeatherDuino']
    for items in data:
      if items['probe'] == 1:
        intemp, inhum = items['temp'], items['humid']
      elif items['probe'] == 2:
        outtemp, outhum = items['temp'], items['humid']
      else:
        print "Ignoring probe with ID %i" % items['probe']
    ProcessRRDdata(destination, "WeatherDuino_temp.rrd", "Temp", "°C",
                   intemp, outtemp)
    ProcessRRDdata(destination, "WeatherDuino_humid.rrd", "Humid", "%",
                   inhum, outhum)
    time.sleep(300)
  
  
if __name__ == '__main__':
  usage = "usage: %prog [options]"
  parser = optparse.OptionParser(usage=usage)
  parser.add_option("-d", "--device", metavar="DEVICE", default="/dev/ttyUSB0",
                    help="The serial device the Arduino is connected to.")
  parser.add_option("-b", "--baud", metavar="BAUD", default=57600, type="int",
                    help="Baudrate at which the Arduino is communicating.")
  parser.add_option("-p", "--path", metavar="PATH", default=None,
                    help="Destination path where the RRD & graphs are written")
  (opts, args) = parser.parse_args()
  try:
    print "%s: Unrecognized argument \'%s\'" % (sys.argv[0], args[0])
    print "Try \'%s --help\' for more information." % sys.argv[0]
    sys.exit(1)
  except IndexError:
    pass
  print "Writing data from device %s at %i baud to '%s'..." % (opts.device, 
                                                               opts.baud,
                                                               opts.path)
  ContinualRRDwrite(GetWeatherDevice(opts.device, opts.baud), opts.path)
