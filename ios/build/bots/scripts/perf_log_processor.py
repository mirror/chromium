# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import math
import re

RESULTS_REGEX = re.compile(r'(?P<IMPORTANT>\*)?RESULT '
                           r'(?P<GRAPH>[^:]*): (?P<TRACE>[^=]*)= '
                           r'(?P<VALUE>[\{\[]?[-\d\., ]+[\}\]]?)('
                           r' ?(?P<UNITS>.+))?')

class PerformanceLogProcessor(object):
  """Class to parse performance results from stdout.

  The log will be parsed looking for any lines of the forms:
    <*>RESULT <graph_name>: <trace_name>= <value> <units>
  or
    <*>RESULT <graph_name>: <trace_name>= [<value>,value,value,...] <units>
  or
    <*>RESULT <graph_name>: <trace_name>= {<mean>, <std deviation>} <units>

  For example,
    *RESULT vm_final_browser: OneTab= 8488 kb
    RESULT startup: ref= [167.00,148.00,146.00,142.00] ms
    RESULT TabCapturePerformance_foo: Capture= {30.7, 1.45} ms

  The leading * is optional; it indicates that the data from that line should
  be considered "important", which may mean for example that it's graphed by
  default.

  If multiple values are given in [], their mean and (sample) standard
  deviation will be written; if only one value is given, that will be written.
  A trailing comma is permitted in the list of values.

  NOTE: All lines except for RESULT lines are ignored.

  The <graph_name> and <trace_name> should not contain any spaces, colons (:)
  nor equals-signs (=). /'s will be replaced by _'s.

  The results will be stored as a list of dicts of the form:
    {
      'test': '<graph_name>/<trace_name>', unless <graph_name> == <trace_name>,
              then only <graph_name> will be used.
      'value': given <value> or <mean> if given multiple <value>s,
      'error': 0 or <std deviation> if given multiple <value>s,
      'important': True if marked as important.
      'units': <units> or '',
    }
  """
  def __init__(self):
    self.traces = []

  def _CalculateStatistics(data):
    n = len(data)
    if n == 0:
      return 0.0, 0.0
    mean = float(sum(data)) / n
    variance = sum([(element - mean)**2 for element in data]) / n
    return mean, math.sqrt(variance)

  def _TestName(chart_name, trace_name):
    trace_name = trace_name.replace('/', '_')
    if chart_name == trace_name:
      return trace_name
    return '%s/%s' % (chart_name, trace_name)

  def ProcessLine(self, line):
    results_match = RESULTS_REGEX.search(line)
    if not results_match:
      return
    match_dict = results_match.groupdict()

    trace = {}
    trace['test'] = self._TestName(match_dict['GRAPH'], match_dict['TRACE'])
    trace['units'] = match_dict['UNITS'] or ''
    trace['important'] = match_dict['IMPORTANT'] or False

    value = match_dict['VALUE']
    # Compute the mean and standard deviation for a multiple-valued item,
    # or the numerical value of a single-valued item.
    try:
      if value[0] == '[':
        trace['value'], trace['error'] = self._CalculateStatistics([
            float(x) for x in value.strip('[],').split(',')
        ])
      elif value[0] == '{':
        trace['value'], trace['error'] = [
            float(x) for x in value.strip('{},').split(',')
        ]
      else:
        trace['value'], trace['error'] = float(value), 0
    except ValueError:
      # Report, but ignore, corrupted data lines. (Lines that are so badly
      # broken that they don't even match the RESULTS_REGEX won't be
      # detected.)
      logging.warning("Bad test output: '%s'" % value.strip())
      return

    self.traces.append(trace)
