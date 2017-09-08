# Copyright (c) 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Performs a diff of the results from resource_sizes.py from two different
   builds, e.g. to compare the size impact of a compile flag.
"""

import argparse
import json
import os
import shutil
import stat
import subprocess
import sys
import tempfile

from pylib.constants import host_paths
from pylib.utils import shared_preference_utils

with host_paths.SysPath(host_paths.BUILD_COMMON_PATH):
    import perf_tests_results_helper # pylint: disable=import-error


_BASE_CHART = {
    'format_version': '0.1',
    'benchmark_name': 'resource_sizes_diff',
    'benchmark_description': 'APK resource size diff information',
    'trace_rerun_options': [],
    'charts': {},
}

_RESULTS_FILENAME = 'results-chart.json'


def DiffResults(chartjson, base_results, diff_results):
  """Reports the diff between the two given results.

  Args:
    chartjson: A dictionary that chartjson results will be placed in, or None
        to only print results.
    base_results: The chartjson-formatted size results of the base APK.
    diff_results: The chartjson-formatted size results of the diff APK.
  """
  for (graph_title, graph) in base_results['charts'].iteritems():
    for (trace_title, trace) in graph.iteritems():
      perf_tests_results_helper.ReportPerfResult(
          chartjson, graph_title, trace_title,
          diff_results['charts'][graph_title][trace_title]['value']
              - trace['value'],
          trace['units'], trace['improvement_direction'],
          trace['important'])


def AddIntermediateResults(chartjson, base_results, diff_results):
  """Copies the intermediate size results into the output chartjson.

  Args:
    chartjson: A dictionary that chartjson results will be placed in.
    base_results: The chartjson-formatted size results of the base APK.
    diff_results: The chartjson-formatted size results of the diff APK.
  """
  if not chartjson:
    return

  for (graph_title, graph) in base_results['charts'].iteritems():
    for (trace_title, trace) in graph.iteritems():
      perf_tests_results_helper.ReportPerfResult(
          chartjson, graph_title + '_base_apk', trace_title,
          trace['value'], trace['units'], trace['improvement_direction'],
          trace['important'])

  # Both base_results and diff_results should have the same charts/traces, but
  # loop over them separately in case they don't
  for (graph_title, graph) in diff_results['charts'].iteritems():
    for (trace_title, trace) in graph.iteritems():
      perf_tests_results_helper.ReportPerfResult(
          chartjson, graph_title + '_diff_apk', trace_title,
          trace['value'], trace['units'], trace['improvement_direction'],
          trace['important'])


def main():
  argparser = argparse.ArgumentParser(
      description='Diff resource sizes of two APKs. Arguments not listed here '
                  'will be passed on to both invocations of resource_sizes.py.')
  argparser.add_argument('--chromium-output-directory-base',
                         dest='out_dir_base',
                         help='Location of the build artifacts for the base '
                              'APK, i.e. what the size increase/decrease will '
                              'be measured from.')
  argparser.add_argument('--chromium-output-directory-diff',
                         dest='out_dir_diff',
                         help='Location of the build artifacts for the diff '
                              'APK.')
  argparser.add_argument('--chartjson',
                         action='store_true',
                         help='Sets output mode to chartjson.')
  argparser.add_argument('--include-intermediate-results',
                         action='store_true',
                         help='Include the results from the resource_sizes.py '
                              'runs in the chartjson output.')
  argparser.add_argument('--output-dir',
                         default='.',
                         help='Directory to save chartjson to.')
  argparser.add_argument('-d', '--device',
                         help='Dummy option for perf runner.')
  argparser.add_argument('--base-apk',
                         required=True,
                         help='Path to the base APK, i.e. what the size '
                              'increase/decrease will be measured from.')
  argparser.add_argument('--diff-apk',
                         required=True,
                         help='Path to the diff APK, i.e. the APK whose size '
                              'increase/decrease will be measured against the '
                              'base APK.')

  (args, unknown_args) = argparser.parse_known_args()
  chartjson = _BASE_CHART.copy() if args.chartjson else None

  base_dir = tempfile.mkdtemp()
  diff_dir = tempfile.mkdtemp()
  # mkdtemp creates a directory that's not usable by subprocesses, so change
  # that
  read_write = (stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR |
                stat.S_IRGRP | stat.S_IWGRP | stat.S_IXGRP |
                stat.S_IROTH | stat.S_IWOTH | stat.S_IXOTH)
  os.chmod(base_dir, read_write)
  os.chmod(diff_dir, read_write)

  try:
    # Run resource_sizes.py on the two APKs
    android_dir = os.path.dirname(os.path.abspath(__file__))
    resource_sizes_path = os.path.join(android_dir, 'resource_sizes.py')
    shared_args = (['/usr/bin/python', resource_sizes_path, '--chartjson']
                   + unknown_args)
    base_args = shared_args + (
        ['--chromium-output-directory', args.out_dir_base] if args.out_dir_base
        else []) + ['--output-dir', base_dir] + [args.base_apk]
    subprocess.check_output(base_args, stderr=subprocess.STDOUT)
    diff_args = shared_args + (
        ['--chromium-output-directory', args.out_dir_diff] if args.out_dir_diff
        else []) + ['--output-dir', diff_dir] + [args.diff_apk]
    subprocess.check_output(diff_args, stderr=subprocess.STDOUT)

    # Combine the separate results
    with open(os.path.join(base_dir, _RESULTS_FILENAME)) as base_file:
      with open(os.path.join(diff_dir, _RESULTS_FILENAME)) as diff_file:
        base_results = shared_preference_utils.UnicodeToStr(
            json.load(base_file))
        diff_results = shared_preference_utils.UnicodeToStr(
            json.load(diff_file))
        DiffResults(chartjson, base_results, diff_results)
        if args.include_intermediate_results:
          AddIntermediateResults(chartjson, base_results, diff_results)

    if args.chartjson:
      with open(os.path.join(os.path.abspath(args.output_dir),
                             _RESULTS_FILENAME), 'w') as outfile:
        json.dump(chartjson, outfile)
  finally:
    shutil.rmtree(base_dir)
    shutil.rmtree(diff_dir)

if __name__ == '__main__':
  sys.exit(main())
