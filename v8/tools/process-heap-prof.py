#!/usr/bin/env python

import csv, sys, time

# Dead simple code borrowed from process-ticks.pl, modified, and later ported
# to Python.
def process_logfile(filename):
  sample_time = 0.0
  sampling = False
  try:
    logfile = open(filename, 'rb')
    try:
      logreader = csv.reader(logfile)

      print('JOB "v8"')
      print('DATE "%s"' % time.asctime(time.localtime()))
      print('SAMPLE_UNIT "seconds"')
      print('VALUE_UNIT "bytes"')

      for row in logreader:
        if row[0] == 'scavenge' and row[1] == 'end':
          sample_time = float(row[2]) + float(row[3])/1000000.0
        elif row[0] == 'heap-sample-begin' and row[1] == 'Heap':
          print('BEGIN_SAMPLE %.2f' % sample_time)
          sampling = True
        elif row[0] == 'heap-sample-end' and row[1] == 'Heap':
          print('END_SAMPLE %.2f' % sample_time)
          sampling = False
        elif row[0] == 'heap-sample-item' and sampling:
          print('%s %d' % (row[1], int(row[3])))
    finally:
      logfile.close()
  except:
    sys.exit('can\'t open %s' % filename)

process_logfile(sys.argv[1])
