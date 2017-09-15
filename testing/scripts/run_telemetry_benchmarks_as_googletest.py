#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Runs an isolate bundled Telemetry benchmark.

This script attempts to emulate the contract of gtest-style tests
invoked via recipes. The main contract is that the caller passes the
argument:

  --isolated-script-test-output=[FILENAME]

json is written to that file in the format detailed here:
https://www.chromium.org/developers/the-json-test-results-format

This script is intended to be the base command invoked by the isolate,
followed by a subsequent Python script. It could be generalized to
invoke an arbitrary executable.
"""

import argparse
import json
import os
import shutil
import sys
import tempfile
import traceback

import common

from core import path_util

import run_telemetry_benchmark_as_googletest

def sharding_map_path():
  return os.path.join(
      path_util.GetChromiumSrcDir(), 'tools', 'perf', 'benchmarks',
      'benchmark_sharding_map.json')

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--isolated-script-test-output', type=argparse.FileType('w'),
      required=True)
  parser.add_argument(
      '--isolated-script-test-chartjson-output', required=False)
  parser.add_argument(
      '--isolated-script-test-perf-output', required=False)
  parser.add_argument('--xvfb', help='Start xvfb.', action='store_true')
  parser.add_argument('--output-format', action='append')
  parser.add_argument('--builder', required=True)
  parser.add_argument('--bot', required=True,
                      help='Bot ID to use to determine which tests to run. Will'
                           ' use //tools/perf/core/benchmark_sharding_map.json'
                           ' with this as a key to determine which benchmarks'
                           ' to execute')
  parser.add_argument(
      '--sharding-json-file',
      help='This json file should contain the information needed to tell each'
      ' shard what to do. The file should be a dictionary mapping test name to'
      ' a list of benchmarks to execute.')
  args, rest_args = parser.parse_known_args()
  for output_format in args.output_format:
    rest_args.append('--output-format=' + output_format)
  isolated_out_dir = os.path.dirname(args.isolated_script_test_output)

  with open(os.path.join(
      path_util.GetChromiumSrcDir(), args.sharding_json_file)) as f:
    sharding_map = json.load(f)
  sharding = sharding_map[args.builder][args.bot]['benchmarks']
  return_code = 0

  for benchmark in sharding:
    per_benchmark_args = [benchmark] + rest_args[:]
    rc, chartresults, json_test_results = (
        run_telemetry_benchmark_as_googletest.run_benchmark(
            args, per_benchmark_args))

    return_code = return_code or rc
    benchmark_path = os.path.join(isolated_out_dir, benchmark)
    with open(os.path.join(benchmark_path, 'chartresults.json')) as f:
      json.dump(chartresults, f)
    with open(os.path.join(benchmark_path, 'test_results.json')) as f:
      json.dump(json_test_results, f)

  return return_code


# This is not really a "script test" so does not need to manually add
# any additional compile targets.
def main_compile_targets(args):
  json.dump([], args.output)


if __name__ == '__main__':
  # Conform minimally to the protocol defined by ScriptTest.
  if 'compile_targets' in sys.argv:
    funcs = {
      'run': None,
      'compile_targets': main_compile_targets,
    }
    sys.exit(common.run_script(sys.argv[1:], funcs))
  sys.exit(main())
