#!/usr/bin/python

import sys

if len(sys.argv) != 2:
  sys.exit('Usage: process-code.py <log>')

class Statistics:
  def __init__(self):
    self.bytes = 0
    self.count = 0

  def Add(self, type, size):
    if (type == 'script') or (type == 'function') or (type == 'lazycompile'):
      self.count += 1
      self.bytes += size
      
  def Dump(self):
    print str(self.count) + ' code objects'
    print str(self.bytes) + ' bytes'
    if self.count > 0:
      print 'Average: ' + str(self.bytes / self.count) + ' bytes per code object'

def ProcessLog(filename, stats):
  for line in open(filename, 'rb').readlines():
    if line.startswith('('):
      line = line[1:len(line)-2]
    entry = [ x.strip().replace('_', '-').lower() for x in line.split(',') ]
    if entry[0] == 'code-creation': stats.Add(entry[1], int(entry[3]))


stats = Statistics()
ProcessLog(sys.argv[1], stats)
stats.Dump()
