#!/usr/bin/python
#
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Gathers native library residency and processes it."""

import argparse
import json
import os
import shutil
import sys
import tempfile
import time

import numpy as np
from matplotlib import pylab as plt
from matplotlib import collections as mc

_SRC_PATH = os.path.abspath(os.path.join(
    os.path.dirname(__file__), os.pardir, os.pardir, os.pardir))
path = os.path.join(_SRC_PATH, 'third_party', 'catapult', 'devil')
sys.path.append(path)
from devil.android import flag_changer
from devil.android import device_errors
from devil.android import device_utils
from devil.android.constants import chrome
from devil.android.perf import cache_control
from devil.android.sdk import intent

sys.path.append(os.path.join(_SRC_PATH, 'build', 'android'))
import devil_chromium


_CHROME = chrome.PACKAGE_INFO['chrome']


def _SetupDevice(device):
  """Sets up the device.

  Enables root, stops Chrome, and clears the results from a previous run.

  Args:
    device: Device to set up.
  """
  if not device.HasRoot():
    device.EnableRoot()
  device.ForceStop(_CHROME.package)
  files = device.ListDirectory('/sdcard/')
  residency_files = [os.path.join('/sdcard/',f)
                     for f in files if f.startswith('residency-')]
  device.RemovePath(residency_files, force=True)


def _Run(device, url, purge, purge_mb):
  """Starts Chrome, loads a URL, outputs data on the device's disk."""
  args = ['--log-residency']
  if purge:
    args.append('--purge')
  if purge_mb:
    assert purge
    args.append('--purge-mb=%d' % purge_mb)

  with flag_changer.CustomCommandLineFlags(device, _CHROME.cmdline_file, args):
    cache_control.CacheControl(device).DropRamCaches()
    start_intent = intent.Intent(
        activity=_CHROME.activity, package=_CHROME.package,data=url)
    device.StartActivity(start_intent)
    time.sleep(20)


def _Collect(device):
  """Collects the results from a device's disk.

  Args:
    device: Device.

  Returns:
    List of files, on the local disk.
  """
  result = []
  residency_files = [os.path.join('/sdcard/',f)
                     for f in device.ListDirectory('/sdcard/')
                     if f.startswith('residency-')]
  directory = tempfile.mkdtemp()
  for f in residency_files:
    filename = os.path.basename(f)
    pulled_filename = os.path.join(directory, filename)
    device.PullFile(f, pulled_filename)
    result.append(pulled_filename)
  return result


def _ParseFile(filename):
  """Parses a result file.

  Returns:
    ((incore, outcore), ranges) where incore is list of pages in core, outcore
    is a list of pages out of core, and ranges is the start of addresse ranges.
  """
  ranges = []
  lines = [line.strip().split() for line in open(filename).readlines()]
  start_incore = [(int(line[0]), line[1]) for line in lines]
  min_address = min(x[0] for x in start_incore)
  max_address = max(x[0] + len(x[1]) * 4096 for x in start_incore)
  (incore, outcore) = ([], [])
  for (start, mincore_result) in start_incore:
    ranges.append(start)
    for (i, x) in enumerate(mincore_result):
      addr = start + i * 4096
      if x == '1':
        incore.append(addr)
      else:
        outcore.append(addr)
  return ((incore, outcore), ranges)


def _ParseFiles(files):
  """Parses the result files.

  Args:
    files: ([str]) List of files, as returned by _Collect()

  Returns:
    ({timestamp: (incore, outcore)}, ranges)
  """
  files_with_timestamp = [(int(f.split('-')[1].split('.')[0]), f)
                          for f in files]
  ranges = []
  result = {}
  for (t, filename) in files_with_timestamp:
    result[t], ranges = _ParseFile(filename)
  shutil.rmtree(os.path.dirname(files[0]))
  return result, ranges


def _Serialize(residency, ranges, filename):
  """Serializes the data to a JSON file.
  """
  json_dict = {'residency': residency, 'ranges': ranges}
  with open(filename, 'w') as output_file:
    json.dump(json_dict, output_file)


def _PlotResidency(data, ranges, output_filename, purge, purge_mb):
  """Plots the residency data.

  Args:
    data: As returned from _ParseFiles()
    ranges: As returned from _ParseFiles()
    output_filename: (str) Output filename
  """
  fig, ax = plt.subplots(figsize=(20, 10))
  x_min = 0
  x_max = 0
  previous_percentage = 0.
  previous_timestamp = 0
  for t in sorted(data.keys()):
    incore, outcore = data[t]
    for (d, color) in ((incore, (.2, .6, .05, 1)), (outcore, (1, 0, 0, 1))):
      segments = [[(x, t), (x + 4096, t)] for x in d]
      colors = np.array([color] * len(segments))
      lc = mc.LineCollection(segments, colors=colors, linewidths=8)
      ax.add_collection(lc)
    if outcore and incore:
      x_min = min(min(incore), min(outcore))
      x_max = max(max(incore), max(outcore)) + 4096
    percentage = 100. * len(incore) / (len(incore) + len(outcore))
    previous_percentage = percentage
    previous_timestamp = t
    plt.text(x_max, t, '%.1f%%' % percentage)

  for start in ranges:
    plt.axvline(start, color='k')

  plt.xlim(xmin=x_min, xmax=x_max)
  plt.ylim(ymin=min(data.keys()) - 100,
           ymax=max(data.keys()) + 100)
  plt.xlabel('Address')
  plt.ylabel('Time (ms)')
  title = 'Resident code pages on startup, prefetching disabled - '
  title += 'Total %.2fMB' % (float(x_max - x_min) / 2**20)
  if purge:
    title += ' - Purge, %d MB' % purge_mb
  else:
    title += ' - No purge'
  plt.title(title)
  plt.savefig(output_filename, dpi=300, bbox_inches='tight')


def main():
  parser = argparse.ArgumentParser(description='Process native lib residency')
  parser.add_argument('--output_filename', type=str, help='Output filename',
                      required=True)
  parser.add_argument('--json_filename', type=str, help='Output JSON filename',
                      required=True)
  parser.add_argument('--device', help='Device ID')
  parser.add_argument('--url', help='URL to load',
                      default='https://www.google.com')
  parser.add_argument('--purge', help='Periodically purge the library',
                      action='store_true')
  parser.add_argument('--purge_mb', help='Size of data to allocate', type=int,
                      default=0)
  parser.add_argument('--run', help='Run the collection', action='store_true',
                      default=False)
  args = parser.parse_args()

  devil_chromium.Initialize()
  devices = device_utils.DeviceUtils.HealthyDevices()
  device = devices[0]
  if len(devices) != 1 and options.device is None:
    logging.error('Several devices attached, must specify one with --device.')
    sys.exit(0)

  if args.run:
    _SetupDevice(device)
    _Run(device, args.url, args.purge, args.purge_mb)
  files = _Collect(device)
  data, ranges = _ParseFiles(files)
  _Serialize(data, ranges, args.json_filename)
  _PlotResidency(data, ranges, args.output_filename, args.purge, args.purge_mb)


if __name__ == '__main__':
  main()
