#!/usr/bin/python2.7
""" RRD Graphing utility for use with the WeatherDuino as found on GitHub:
https://github.com/Fludizz/WeatherDuino
It parses the serial output and writes it to an RRD database file and generates
the new graphs.

More information on how to build this device:
http://frack.nl/wiki/WeatherDuino """
__author__ = 'Rudi Daemen <fludizz@gmail.com>'
__version__ = '0.2'

import rrdtool
import sys
import optparse
import ConfigParser
import os
import serial
import time
try:
    import simplejson as json
except ImportError:
    import json


# MRTG uses:
# 1 week graphs 30m averages: 6 steps / 800 rows
# 1 Month graphs with 2h avarages: 24 steps / 800 rows
# 1 year graphs with 1 day avarage: 288 steps / 800 rows
# Extracted from mrtg generated RRD file using 'rrdtool info <rrdfile>'.
# Assuming MRTG knows what it's doing, we assume the same values.
DEFRRD = [ "DS:P1:GAUGE:600:-100:100",
           "DS:P2:GAUGE:600:-100:100",
           "DS:P3:GAUGE:600:-100:100",
           "DS:P4:GAUGE:600:-100:100",
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


def UpdateRRDfile(path, rrd, val, defs=DEFRRD):
  ''' This function only writes collected data to the RRD File but does NOT 
  write new images. This increases the data detail for the daily graphs. This
  does not need to be called more often however due to the Humidity probes being
  relativly inaccurate (only whole percents at a time), this causes a smoother
  graph for the humidity data. '''
  rrdfile = os.path.abspath("%s/%s" % (path, rrd))
  # Check if the rrdfile exists and create if needed.
  if not os.path.isfile(rrdfile):
    print "INFO: RRD file %s does not exist. Creating a new RRD file." % rrdfile
    rrdtool.create(rrdfile, defs)
  # update the data in the rrdfile:
  rrdtool.update(rrdfile, "N:%s" % ":".join(map(str, val)))


def ProcessRRDdata(path, rrd, prefix, name, axis_unit):
  ''' This function updates the graphs using the RRDfile at path/rrd. It will
  always generate a new daily graph on each run. However, it will only create
  the weekly/monthly/yearly graphs once every half their avaraging-time. '''
  rrdfile = os.path.abspath("%s/%s" % (path, rrd))
  imgname = os.path.abspath("%s/%s_%s" % (path, prefix, name))
  # Dirty fix for the escaping mismatch... RRDtool uses the % sign in *some*
  # parameters as the escaping character but not in the axis unit. This cause
  # the escape value to break the rrd graph generation. If the unit is %, make
  # it an escaped % sign ('%%') and don't do this for the axis.
  unit = axis_unit.replace('%', '%%')

  # Generate graphs for each configured Delta in TIMEDELTA:
  for delta in TIMEDELTA.keys():
    # All graphs for different timedelta's are generate at different minutes to
    # reduce the CPU overhead while generating the images. Usefull for low power
    # devices like a Raspberry Pi.
    if delta == "day" and int(time.strftime("%M")) % 5 == 0:
      # Generate the daily graphs every 5 minutes.
      gen_graph = True
    elif delta == "week" and int(time.strftime("%M")) in (5, 20, 35, 50):
      # Generate the Weekly graph every 15 minutes (at minutes 5, 20, 35 and 50)
      gen_graph = True
    elif delta == "month" and int(time.strftime("%M")) == 55:
      # Generate the Monthly graphs once every hour (at minute 55).
      gen_graph = True
    elif delta == "year" and int(time.strftime("%H")) in (0, 12):
      # Generate the yearly graphs once every 12 hours (at 12:00 and 0:00).
      gen_graph = True
    else:
      gen_graph = False
    if gen_graph:
      rrdtool.graph(
          "%s-%s.png" % (imgname, delta), 
          "--start", TIMEDELTA[delta],
          "--vertical-label=%s (%s)" % (name, axis_unit),
          "--slope-mode",
          "--font", "LEGEND:7",
          "DEF:P1avg=%s:P1:AVERAGE" % rrdfile,
          "DEF:P2avg=%s:P2:AVERAGE" % rrdfile,
          "DEF:P3avg=%s:P3:AVERAGE" % rrdfile,
          "DEF:P4avg=%s:P4:AVERAGE" % rrdfile,
          "DEF:P1min=%s:P1:MIN" % rrdfile,
          "DEF:P2min=%s:P2:MIN" % rrdfile,
          "DEF:P3min=%s:P3:MIN" % rrdfile,
          "DEF:P4min=%s:P4:MIN" % rrdfile,
          "DEF:P1max=%s:P1:MAX" % rrdfile,
          "DEF:P2max=%s:P2:MAX" % rrdfile,
          "DEF:P3max=%s:P3:MAX" % rrdfile,
          "DEF:P4max=%s:P4:MAX" % rrdfile,
          "TEXTALIGN:left",
          "LINE1:P1avg#00CC00:Probe1\::",
          "GPRINT:P1avg:LAST:%%.1lf%s\\t" % unit,
          "GPRINT:P1max:MAX:Max\: %%.1lf%s\\t" % unit,
          "GPRINT:P1avg:AVERAGE:Avg\: %%.1lf%s\\t" % unit,
          "GPRINT:P1min:MIN:Min\: %%.1lf%s\\l" % unit,
          "LINE1:P2avg#FF9900:Probe2\::",
          "GPRINT:P2avg:LAST:%%.1lf%s\\t" % unit,
          "GPRINT:P2max:MAX:Max\: %%.1lf%s\\t" % unit,
          "GPRINT:P2avg:AVERAGE:Avg\: %%.1lf%s\\t" % unit,
          "GPRINT:P2min:MIN:Min\: %%.1lf%s\\l" % unit,
          "LINE1:P3avg#9900FF:Probe3\::",
          "GPRINT:P3avg:LAST:%%.1lf%s\\t" % unit,
          "GPRINT:P3max:MAX:Max\: %%.1lf%s\\t" % unit,
          "GPRINT:P3avg:AVERAGE:Avg\: %%.1lf%s\\t" % unit,
          "GPRINT:P3min:MIN:Min\: %%.1lf%s\\l" % unit,
          "LINE1:P4avg#0000CC:Probe4\::",
          "GPRINT:P4avg:LAST:%%.1lf%s\\t" % unit,
          "GPRINT:P4max:MAX:Max\: %%.1lf%s\\t" % unit,
          "GPRINT:P4avg:AVERAGE:Avg\: %%.1lf%s\\t" % unit,
          "GPRINT:P4min:MIN:Min\: %%.1lf%s\\l" % unit)


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
    print "%s (%sbps): WeatherDuino initialized!" % (device,baud)
    return weatherduino
  else:
    raise Exception('Device %s (%sbps) is not a WeatherDuino!' % (device, baud))


def ContinualRRDwrite(config):
  ''' Open the WeatherDuino device, run an endless loop for collecting and 
  writing weather data. '''
  if config["files"]["path"].startswith("~"):
    path = os.path.expanduser(config["files"]["path"])
  else:
    path = os.path.abspath(config["files"]["path"])
  if not os.path.exists(path):
    raise Exception('Destination path \'%s\' does not exist.' % path)
  prefix = config["files"]["prefix"]
  print "Writing weatherdata to files '%s/%s*'" % (path, prefix)
  humidrrd = "%s_humid.rrd" % prefix
  temprrd = "%s_temp.rrd" % prefix

  oldtime = None
  # Initialize the WeatherDuino!
  arduino = GetWeatherDevice(config["device"]["port"], config["device"]["baud"])
  # clear any potential junk from the buffer
  arduino.readline()

  # Make an empty list with NUM devices (where NUM is in config).
  # In theses lists, probe is position + 1 (e.g. temps[0] is probe 1).
  temps = [ None ] * int(config['device']['num'])
  hums = [ None ] * int(config['device']['num'])
  while True:
    try:
      data = json.loads(arduino.readline().strip())['WeatherDuino']
    except serial.serialutil.SerialException, err:
      print "[%s] Error: %s" % (time.ctime(), err)
      sys.exit(1)
      pass
    # Only process the data once per 5 minutes but keep reading the buffer. This
    # constant reading is required because otherwise the serial device times out
    for items in data:
      try:
        if items['probe'] <= int(config['device']['num']):
          # Probes are numberd 1 through 4, need to adjust to match the list!
          pos = items['probe'] - 1
          temps[pos] = items['temp']
          hums[pos] = items['humid']
          print "[%s] Probe %s: T%s H%s;" % (time.ctime(), items['probe'],
                                             temps[pos], hums[pos])
        else:
          print "Ignoring unconfigured probe with ID '%s'" % items['probe']
      except KeyError, err:
        print "Ignoring invalid key %s" % err
        pass

    # Update RRDfile regardless of the graphs/time
    UpdateRRDfile(path, temprrd, temps)
    UpdateRRDfile(path, humidrrd, hums)

    # every 5 minutes, generate graphs:
    newtime = int(time.strftime('%M'))
    if not newtime == oldtime and newtime % 5 == 0:
      oldtime = newtime
      ProcessRRDdata(path, temprrd, prefix, "temp", u"\u00B0C".encode('utf8'))
      ProcessRRDdata(path, humidrrd, prefix, "humid", u"%".encode('utf8'))


if __name__ == '__main__':
  usage = "usage: %prog [options]"
  parser = optparse.OptionParser(usage=usage)
  parser.add_option("-c", "--conf", metavar="CONF", default=None,
                    help="""Specify configfile to use. Note: When using a config
                     file, all other options are ignored.""")
  parser.add_option("-d", "--device", metavar="DEVICE", default="/dev/ttyUSB0",
                    help="The serial device the Arduino is connected to.")
  parser.add_option("-b", "--baud", metavar="BAUD", default=57600, type="int",
                    help="Baudrate at which the Arduino is communicating.")
  parser.add_option("-n", "--num", metavar="NUM", default=4, type="int",
                    help="Number of probes connected to WeatherDuino")
  parser.add_option("-p", "--path", metavar="PATH", default=".",
                    help="Destination path where the RRD & graphs are written")
  parser.add_option("-x", "--prefix", metavar="PREFIX", default="WeatherDuino",
                    help="Filename prefix for the RRD files.")
  (opts, args) = parser.parse_args()
  try:
    print "%s: Unrecognized argument \'%s\'" % (sys.argv[0], args[0])
    print "Try \'%s --help\' for more information." % sys.argv[0]
    sys.exit(1)
  except IndexError:
    pass
# store the received options in config dictionary
  config = { "device": { "port": opts.device,
                         "baud": opts.baud,
                         "num": opts.num },
             "files": { "path": opts.path,
                        "prefix": opts.prefix } }
# Check for config file and overwrite the config using this file.
  if opts.conf:
    # Some basic path expansion
    if opts.conf.startswith("~"):
      conffile = os.path.expanduser(opts.conf)
    else:
      conffile = os.path.abspath(opts.conf)
    # Check if the expanded config file is there and parse it!
    if os.path.isfile(conffile):
      parser = ConfigParser.RawConfigParser()
      parser.read(conffile)
      config = dict((section, dict(parser.items(section)))
                    for section in parser.sections())
      print "Using configfile '%s' - Ignoring all other arguments." % conffile
    else:
      print "Configfile '%s' not found or not accessible!" % conffile
      sys.exit(1)

  print "Using '%s' ('%s' baud) with %s Probes..." % (config['device']['port'],
                                                      config['device']['baud'],
                                                      config['device']['num'])
  ContinualRRDwrite(config)
