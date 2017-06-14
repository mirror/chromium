# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Visits various WebVR app URLs and records performance metrics from VrCore.
"""

import android_vr_perf_test
import vr_perf_summary

import json
import logging
import numpy
import os
import time

DEFAULT_URLS = [
  # Standard WebVR sample app with no changes
  'https://webvr.info/samples/test-slow-render.html?'
  'canvasClickPresents=1\&renderScale=1',
  # Increased render scale
  'https://webvr.info/samples/test-slow-render.html?'
  'canvasClickPresents=1\&renderScale=1.5',
  # Default render scale, but increased load
  'https://webvr.info/samples/test-slow-render.html?'
  'canvasClickPresents=1\&'
  'renderScale=1\&heavyGpu=1\&cubeScale=0.2\&workTime=5',
  # Further increased load
  'https://webvr.info/samples/test-slow-render.html?'
  'canvasClickPresents=1\&'
  'renderScale=1\&heavyGpu=1\&cubeScale=0.3\&workTime=10',
]


class VrCoreFpsTest(android_vr_perf_test.AndroidVrPerfTest):
  def __init__(self, args):
    super(VrCoreFpsTest, self).__init__(args)
    self._duration = args.duration
    assert (self._duration > 0),'Duration must be positive'
    self._test_urls = args.urls or DEFAULT_URLS
    self._test_results = {}

  def _Run(self, url):
    # We're already in VR and logcat is cleared during setup, so wait for
    # the test duration
    time.sleep(self._duration)

    # Exit VR so that VrCore stops logging performance data
    self._Adb(['shell', 'input', 'keyevent', 'KEYCODE_BACK'])
    time.sleep(1)

    output = self._Adb(['logcat', '-d'])
    logging_sessions = vr_perf_summary.ParseLinesIntoSessions(output)
    if len(logging_sessions) != 1:
      raise RuntimeError('Expected 1 VR session, found %d' %
                         len(logging_sessions))
    session = logging_sessions[0]
    if len(session) == 0:
      raise RuntimeError('No data actually collected in logging session')
    self._StoreResults(url, vr_perf_summary.ComputeSessionStatistics(session))

  def _StoreResults(self, url, results):
    """Temporarily stores the results of a test.

    Stores the given results in memory to be later retrieved and written to
    a file in _SaveResultsToFile once all tests are done. Also logs the raw
    data and calculated statistics.
    """
    logging.info('\nURL: %s\n'
                 'Raw app FPS: %s\n'
                 'Raw asynchronous reprojection thread FPS: %s\n'
                 'Raw asynchronous reprojection thread missed frames: %s\n'
                 'Raw frame submissions blocked on gpu: %s\n'
                 '%s\n' %
                 (url, str(results[vr_perf_summary.APP_FPS_KEY]),
                  str(results[vr_perf_summary.ASYNC_FPS_KEY]),
                  str(results[vr_perf_summary.ASYNC_MISSED_KEY]),
                  str(results[vr_perf_summary.BLOCKED_SUBMISSION_KEY]),
                  vr_perf_summary.StringifySessionStatistics(results)))
    self._test_results[url] = results

  def _SaveResultsToFile(self):
    if not (self._args.output_dir and os.path.isdir(self._args.output_dir)):
      logging.warning('No output directory set, not saving results to file')
      return

    # TODO(bsheedy): Move this to a common place so other tests can use it
    def _GenerateTrace(name, improvement_direction, std=None,
                       value_type=None, units=None, values=None):
      return {
          'improvement_direction': improvement_direction,
          'name': name,
          'std': std or 0.0,
          'type': value_type or 'list_of_scalar_values',
          'units': units or '',
          'values': values or [],
      }

    def _GenerateBaseChart(name, improvement_direction, std=None,
                           value_type=None, units=None, values=None):
      return {'summary': _GenerateTrace(name, improvement_direction, std=std,
                                        value_type=value_type, units=units,
                                        values=values)}

    app_fps_string = self._device_name + '_app_fps'
    async_fps_string = self._device_name + '_async_thread_fps'
    async_missed_string = self._device_name + '_async_thread_missed_vsyncs'
    blocked_submission_string = self._device_name + '_blocked_frame_submissions'

    charts = {
        app_fps_string:
            _GenerateBaseChart(app_fps_string, 'up', units='FPS'),
        async_fps_string:
            _GenerateBaseChart(async_fps_string, 'up', units='FPS'),
        async_missed_string:
            _GenerateBaseChart(async_missed_string, 'down', units='us'),
        blocked_submission_string:
            _GenerateBaseChart(blocked_submission_string, 'down'),
    }

    for url, results in self._test_results.iteritems():
      result = results[vr_perf_summary.APP_FPS_KEY]
      charts[app_fps_string][url] = (
          _GenerateTrace(app_fps_string, 'up', units='FPS',
                         std=numpy.std(result), values=result))
      charts[app_fps_string]['summary']['values'].extend(result)

      result = results[vr_perf_summary.ASYNC_FPS_KEY]
      charts[async_fps_string][url] = (
          _GenerateTrace(async_fps_string, 'up', units='FPS',
                         std=numpy.std(result), values=result))
      charts[async_fps_string]['summary']['values'].extend(result)

      result = results[vr_perf_summary.ASYNC_MISSED_KEY]
      charts[async_missed_string][url] = (
          _GenerateTrace(async_missed_string, 'down', units='us',
                         # Missed async reprojection thread vsyncs are only
                         # logged when they happen, unlike the rest of the data
                         # so it's normal to get an empty list, which makes
                         # numpy unhappy
                         std=(0.0 if len(result) == 0 else numpy.std(result)),
                         values=result))
      charts[async_missed_string]['summary']['values'].extend(result)

      result = results[vr_perf_summary.BLOCKED_SUBMISSION_KEY]
      charts[blocked_submission_string][url] = (
          _GenerateTrace(blocked_submission_string, 'down',
                         std=numpy.std(result), values=result))
      charts[blocked_submission_string]['summary']['values'].extend(result)

    results = {
        'format_version': '1.0',
        'benchmarck_name': 'vrcore_fps',
        'benchmark_description': 'Measures VR FPS using VrCore perf logging',
        'charts': charts,
    }

    with file(os.path.join(self._args.output_dir,
                           self._args.results_file), 'w') as outfile:
      json.dump(results, outfile)
