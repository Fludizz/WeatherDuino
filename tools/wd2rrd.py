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
import time
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


def ProcessRRDdata(path, rrd, name, axis_unit, inval, outval, defs=DEFRRD):
  ''' This function checks (and optionally creates) the required rrd file and
  will update this file with the data provided. After updating the file, it will
  generate the required graphs. '''
  if path:
    rrdfile = "%s/%s" % (path, rrd)
    imgname = "%s/%s" % (path, name)
  else:
    rrdfile = rrd
    imgname = name
  # Dirty fix for the escaping mismatch... RRDtool uses the % sign in *some*
  # parameters as the escaping character but not in the axis unit. This cause
  # the escape value to break the rrd graph generation. If the unit is %, make
  # it an escaped % sign ('%%') and don't do this for the axis.
  unit = axis_unit.replace('%', '%%')
  # Check if the rrdfile exists and create if needed.
  if not os.path.isfile(rrdfile):
    rrdtool.create(rrdfile, defs)
  # update the data in the rrdfile:
  rrdtool.update(rrdfile, "N:%s:%s" % (inval, outval))
  # Generate graphs for each configured Delta in TIMEDELTA:
  for delta in TIMEDELTA.keys():
    if delta == "day":
      rrdtool.graph(
          "%s-%s.png" % (imgname, delta), 
          "--start", TIMEDELTA[delta],
          "--vertical-label=%s (%s)" % (name, axis_unit),
          "--slope-mode",
          "DEF:indoor=%s:In:AVERAGE" % rrdfile,
          "DEF:outdoor=%s:Out:AVERAGE" % rrdfile,
          "LINE1:indoor#00FF00:Indoor\:",
          "GPRINT:indoor:AVERAGE:%%6.1lf%s" % unit,
          "LINE1:outdoor#0000FF:Outdoor\:",
          "GPRINT:outdoor:AVERAGE:%%6.1lf%s\\r" % unit)
    else:
      # Only generate the "slower" graphs every 30m.
      if int(time.strftime('%M')) % 30 == 0:
        rrdtool.graph(
            "%s-%s.png" % (imgname, delta), 
            "--start", TIMEDELTA[delta],
            "--vertical-label=%s (%s)" % (name, axis_unit),
            "--slope-mode",
            "DEF:inavg=%s:In:AVERAGE" % rrdfile,
            "DEF:outavg=%s:Out:AVERAGE" % rrdfile,
            "DEF:inmin=%s:In:MIN" % rrdfile,
            "DEF:outmin=%s:Out:MIN" % rrdfile,
            "DEF:inmax=%s:In:MAX" % rrdfile,
            "DEF:outmax=%s:Out:MAX" % rrdfile,
            "LINE1:inmax#FFFF00:In Max:",
            "GPRINT:inmax:MAX:%%6.1lf%s" % unit,
            "LINE1:outmax#00FFFF:Out Max:",
            "GPRINT:outmax:MAX:%%6.1lf%s\\r" % unit,
            "LINE1:inavg#00FF00:In Avg:",
            "GPRINT:inavg:AVERAGE:%%6.1lf%s" % unit,
            "LINE1:outavg#0000FF:Out Avg:",
            "GPRINT:outavg:AVERAGE:%%6.1lf%s\\r" % unit,
            "LINE1:inmin#AAFF00:In Min:",
            "GPRINT:inmin:MIN:%%6.1lf%s" % unit,
            "LINE1:outmin#00AAFF:Out Min:",
            "GPRINT:outmin:MIN:%%6.1lf%s\\r" % unit)


def GetWeatherDevice(device="/dev/ttyUSB0", baud=57600):
  ''' Generate a device/object useable for the program, if it is a valid device
  (e.g. a WeatherDuino). '''
  weatherduino = serial.Serial(device, baud, timeout=30)
  weatherduino.setDTR(True)
  weatherduino.setDTR(False)
  # Clear the initial junk from the buffer
  weatherduino.flushInput()
  weatherduino.readline()
  if "WeatherDuino" in json.loads(weatherduino.readline()):
    print "Found WeatherDuino device on %s at %s." % (device,baud)
    return weatherduino
  else:
    raise Exception('Device is not a WeatherDuino!')


def ContinualRRDwrite(arduino, fdestination, fprefix):
  ''' With the returned WeatherDuino device, run an endless loop collecting and
  writing weather data. '''
  if not os.path.exists(fdestination):
    raise Exception('Destination path does not exist.')
  print "Writing weatherdata to files '%s%s*'" % (fdestination,fprefix)
  oldtime = None
  while True:
    arduino.readline()
    # Only process the data once per 5 minutes but keep reading the buffer. This
    # constant reading is required because otherwise the serial device times out
    newtime = int(time.strftime('%M'))
    if not newtime == oldtime and newtime % 5 == 0:
      oldtime = newtime
      data = arduino.readline().strip()
      for items in json.loads(data)['WeatherDuino']:
        if items['probe'] == 1:
          intemp, inhum = items['temp'], items['humid']
        elif items['probe'] == 2:
          outtemp, outhum = items['temp'], items['humid']
        else:
          print "Ignoring probe with ID %i" % items['probe']
      print "[%s] Probe 1: T%s H%s; Probe 2: T%s H%s" % (time.ctime(), intemp, 
                                                         inhum, outtemp, outhum)
      ProcessRRDdata(fdestination, "%s_temp.rrd" % fprefix, "Temp", 
                     u"\u00B0C".encode('utf8'), intemp, outtemp)
      ProcessRRDdata(fdestination, "%s_humid.rrd" % fprefix, "Humid", 
                     u"%".encode('utf8'), inhum, outhum)
  
  
if __name__ == '__main__':
  usage = "usage: %prog [options]"
  parser = optparse.OptionParser(usage=usage)
  parser.add_option("-d", "--device", metavar="DEVICE", default="/dev/ttyUSB0",
                    help="The serial device the Arduino is connected to.")
  parser.add_option("-b", "--baud", metavar="BAUD", default=57600, type="int",
                    help="Baudrate at which the Arduino is communicating.")
  parser.add_option("-p", "--path", metavar="PATH", default="./",
                    help="Destination path where the RRD & graphs are written")
  parser.add_option("-f", "--file", metavar="FILE", default="WeatherDuino",
                    help="Filename prefix for the RRD files.")
  (opts, args) = parser.parse_args()
  try:
    print "%s: Unrecognized argument \'%s\'" % (sys.argv[0], args[0])
    print "Try \'%s --help\' for more information." % sys.argv[0]
    sys.exit(1)
  except IndexError:
    pass
  weatherduino = GetWeatherDevice(opts.device, opts.baud)
  ContinualRRDwrite(weatherduino, opts.path, opts.file)
