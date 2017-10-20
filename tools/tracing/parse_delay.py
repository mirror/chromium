#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import sys
from pprint import pprint
# sudo apt-get install python-matplotlib
import matplotlib.pyplot as plt
import os

CAT_DELAY = u'disabled-by-default-toplevel.flow'
CAT_METADATA = u'__metadata'
ARG_DELAY = u'queue_duration'
EVENT_NAME = u'MessageLoop::PostTask'

# Thread names
BROWSER_THREAD_NAME = u'CrBrowserMain'
GPU_THREAD_NAME = u'CrGpuMain'

def FindThreadMetadata(events, thread_name):
  for event in events:
    if event['cat'] != CAT_METADATA or event['name'] != u'thread_name':
      continue

    if event['args']['name'] == thread_name:
      return (event['pid'], event['tid'])

  sys.stderr.write("Can't find %s metadata.\n" % thread_name)
  exit(1)

def FindMatchingEvents(events, thread_name):
  find_pid, find_tid = FindThreadMetadata(events, thread_name)
  matching_events = []
  for event in events:
    if event['pid'] != find_pid or event['tid'] != find_tid:
      continue

    if event['name'] != EVENT_NAME or event['cat'] != CAT_DELAY:
      continue

    if ARG_DELAY not in event['args']:
      continue

    time = event['ts'] / 1000000.0
    delay = event['args'][ARG_DELAY]

    matching_events.append((time, delay))

  matching_events.sort()
  return matching_events

def PrintMatchingEvents(matching_events, start_time):
  print('time(ms),delay(ms)')
  for event in matching_events:
    # Time relative to first event in seconds.
    adjusted_time = event[0] - start_time
    print('%f,%d' % (adjusted_time, event[1]))

def PlotEvents(events, start_time, line_color, label):
  x_time = []
  y_delay = []

  for event in events:
    # Time relative to first event in seconds.
    x_time.append(event[0] - start_time)
    # Queuing delay.
    y_delay.append(event[1])

  line = plt.plot(x_time, y_delay, label=label)
  line[0].set_antialiased(True)
  plt.setp(line, color=line_color, linewidth=0.5)

def main():
  if len(sys.argv) == 1 or sys.argv[1] == "":
    sys.stderr.write("No input file specified\n")
    return 1

  filename = sys.argv[1];

  with open(filename) as data_file:
    data = json.load(data_file)

  events = data['traceEvents']
  browser_events = FindMatchingEvents(events, BROWSER_THREAD_NAME)
  gpu_events = FindMatchingEvents(events, GPU_THREAD_NAME)

  start_time = min(browser_events[0][0], gpu_events[0][0])

  PlotEvents(browser_events, start_time, 'r', 'browser')
  PlotEvents(gpu_events, start_time, 'b', 'gpu')

  plt.xlabel('Time (s)')
  plt.ylabel('Delay (ms)')
  plt.title(os.path.basename(filename))
  plt.legend()
  plt.show()

  return 0

if __name__ == '__main__':
  sys.exit(main())
