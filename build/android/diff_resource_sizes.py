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

with host_paths.SysPath(host_paths.BUILD_COMMON_PATH):
    import perf_tests_results_helper # pylint: disable=import-error


_BASE_CHART = {
    'format_version': '0.1',
    'benchmark_name': 'resource_sizes_diff',
    'benchmark_description': 'APK resource size diff information',
    'trace_rerun_options': [],
    'charts': {},
}


# TODO(bsheedy): Move this to a common file (pylib?) instead of copying from
# resource_sizes.py
def ReportPerfResult(chart_data, graph_title, trace_title, value, units,
                     improvement_direction='down', important=True):
  """Outputs test results in correct format.

  If chart_data is None, it outputs data in old format. If chart_data is a
  dictionary, formats in chartjson format. If any other format defaults to
  old format.
  """
  if chart_data and isinstance(chart_data, dict):
    chart_data['charts'].setdefault(graph_title, {})
    chart_data['charts'][graph_title][trace_title] = {
        'type': 'scalar',
        'value': value,
        'units': units,
        'improvement_direction': improvement_direction,
        'important': important
    }
  else:
    perf_tests_results_helper.PrintPerfResult(
        graph_title, trace_title, [value], units)


def DiffResults(chartjson, base_results, diff_results):
  # TODO(bsheedy): Use the version in shared_preference_utils
  def unicode_to_str(data):
    if isinstance(data, dict):
      return {unicode_to_str(key): unicode_to_str(value)
              for key, value in data.iteritems()}
    elif isinstance(data, list):
      return [unicode_to_str(element) for element in data]
    elif isinstance(data, unicode):
      return data.encode('utf-8')
    return data

  base_results = unicode_to_str(base_results)
  diff_results = unicode_to_str(diff_results)

  for (graph_title, graph) in base_results['charts'].iteritems():
    for (trace_title, trace) in graph.iteritems():
      ReportPerfResult(chartjson, graph_title, trace_title,
          diff_results['charts'][graph_title][trace_title]['value']
              - trace['value'],
          trace['units'], trace['improvement_direction'],
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
  # mkdtemp creates a directory that's not usable by subprocesses, so change
  # that
  read_write = (stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR |
                stat.S_IRGRP | stat.S_IWGRP | stat.S_IXGRP |
                stat.S_IROTH | stat.S_IWOTH | stat.S_IXOTH)
  os.chmod(base_dir, read_write)
  diff_dir = tempfile.mkdtemp()
  os.chmod(diff_dir, read_write)

  try:
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
    with open(os.path.join(base_dir, 'results-chart.json')) as base_file:
      with open(os.path.join(diff_dir, 'results-chart.json')) as diff_file:
        DiffResults(chartjson, json.load(base_file), json.load(diff_file))
  finally:
    shutil.rmtree(base_dir)
    shutil.rmtree(diff_dir)

if __name__ == '__main__':
  sys.exit(main())
