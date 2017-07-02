#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Validator for the //testing/buildbot/*.json files."""

from __future__ import print_function

import argparse
import glob
import json
import os
import sys


from schema import And, Schema, Optional, Or, Use


_cipd_schema = Schema({
    'cipd_package': unicode,
    'location': unicode,
    'revision': unicode,
})

_dimension_schema = Schema({
    Optional('android_devices'): And(unicode, Use(int)),
    Optional('device_os'): unicode,
    Optional('device_type'): unicode,
    Optional('gpu'): unicode,
    Optional('hidpi'): '1',
    Optional('id'): unicode,
    Optional('os'): unicode,
    Optional('pool'): unicode
})


_merge_schema = Schema({
    'args': [unicode],
    'script': unicode,
})


_swarming_schema = Schema({
    Optional('can_use_on_swarming_builders'): bool,
    Optional('cipd_packages'): [_cipd_schema],
    Optional('dimension_sets'): [_dimension_schema],
    Optional('expiration'): Or(int, And(unicode, Use(int))),
    Optional('hard_timeout'): Or(int, And(unicode, Use(int))),
    Optional('ignore_task_failure'): bool,
    Optional('io_timeout'): int,
    Optional('output_links'): [{'link': [unicode], 'name': unicode}],
    Optional('shards'): int,
})


_gtest_schema = Schema({
    Optional('args'): [unicode],
    Optional('hard_timeout'): int,
    Optional('merge'): _merge_schema,
    Optional('name'): unicode,
    Optional('override_isolate_target'): unicode,
    Optional('swarming'): _swarming_schema,
    'test': unicode,
    Optional('use_xvfb'): bool,
})


_instrumentation_test_schema = Schema({
    Optional('args'): [unicode],
    Optional('name'): unicode,
    'test': unicode,
})


_isolated_script_schema = Schema({
    Optional('args'): [unicode],
    Optional('isolate_name'): unicode,
    Optional('override_compile_targets'): [unicode],
    Optional('results_handler'): unicode,
    Optional('name'): unicode,
    Optional('merge'): _merge_schema,
    Optional('non_precommit_args'): [unicode],
    Optional('precommit_args'): [unicode],
    Optional('script'): unicode,
    Optional('swarming'): _swarming_schema,
})


_junit_schema = Schema({
    Optional('name'): unicode,
    'test': unicode,
})


_script_schema = Schema({
    Optional('args'): [unicode],
    'name': unicode,
    'script': unicode,
})


_builder_schema = Schema({
  Optional('additional_compile_targets'): [unicode],
  Optional('disabled_tests'): {unicode: unicode},
  Optional('gtest_tests'): [Or(unicode, _gtest_schema)],
  Optional('isolated_scripts'): [_isolated_script_schema],
  Optional('instrumentation_tests'): [_instrumentation_test_schema],
  Optional('junit_tests'): [_junit_schema],
  Optional('scripts'): [_script_schema],
})


_master_schema = Schema({Optional(unicode): _builder_schema})


def validate_master(data):
  return _master_schema.validate(data)


def validate_builder(data):
  return _builder_schema.validate(data)


def validate_file(path):
  with open(path) as fp:
    return validate_master(json.load(fp))


def paths(skip_generated=False):
  testing_dir = os.path.dirname(__file__)
  files_to_skip = [
      'chromium.angle.json',
      'trybot_analyze_config.json']
  if skip_generated:
    files_to_skip += [
      'chromium.gpu.json',
      'chromium.gpu.fyi.json',
      'chromium.perf.json',
      'chromium.perf.fyi.json',
    ]
  return [path for path in glob.glob(os.path.join(testing_dir, '*.json'))
          if not os.path.basename(path) in files_to_skip]


def parse_args(argv, desc):
  parser = argparse.ArgumentParser(description=desc)
  parser.add_argument('--skip-generated', action='store_true',
                      help='Skip the generated files like chromium.gpu.json.')
  parser.add_argument('-v', '--verbose', action='store_true')
  parser.add_argument('path', nargs='*',
                      help='Path to buildbot json file to validate.')
  args = parser.parse_args(argv)
  if not args.path:
    args.path = paths(args.skip_generated)
  return args

def main(argv):
  args = parse_args(argv, __doc__)
  exit_code = 0
  for path in args.path:
    try:
      validate_file(path)
      if args.verbose:
        print('%s passed.' % path)
    except Exception as e:
      print('%s failed:')
      print(str(e))
      exit_code = 1

  return exit_code


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
