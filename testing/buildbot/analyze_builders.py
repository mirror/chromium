#!/usr/bin/env python

import glob
import json
import os
import sys
import urllib2

from schema import And, Schema, Optional, Or, Use

cipd_schema = Schema({
    'cipd_package': unicode,
    'location': unicode,
    'revision': unicode,
})

dimension_schema = Schema({
    Optional('android_devices'): And(unicode, Use(int)),
    Optional('device_os'): unicode,
    Optional('device_type'): unicode,
    Optional('gpu'): unicode,
    Optional('hidpi'): '1',
    Optional('id'): unicode,
    Optional('os'): unicode,
    Optional('pool'): unicode
})

merge_schema = Schema({
    'args': [unicode],
    'script': unicode,
})

swarming_schema = Schema({
    Optional('can_use_on_swarming_builders'): bool,
    Optional('cipd_packages'): [cipd_schema],
    Optional('dimension_sets'): [dimension_schema],
    Optional('expiration'): Or(int, And(unicode, Use(int))),
    Optional('hard_timeout'): Or(int, And(unicode, Use(int))),
    Optional('ignore_task_failure'): bool,
    Optional('io_timeout'): int,
    Optional('output_links'): [{'link': [unicode], 'name': unicode}],
    Optional('shards'): int,
})

gtest_schema = Schema({
    Optional('args'): [unicode],
    Optional('hard_timeout'): int,
    Optional('merge'): merge_schema,
    Optional('name'): unicode,
    Optional('override_isolate_target'): unicode,
    Optional('swarming'): swarming_schema,
    'test': unicode,
    Optional('use_xvfb'): bool,
})

instrumentation_test_schema = Schema({
    Optional('args'): [unicode],
    Optional('name'): unicode,
    'test': unicode,
})

isolated_script_schema = Schema({
    Optional('args'): [unicode],
    Optional('isolate_name'): unicode,
    Optional('override_compile_targets'): [unicode],
    Optional('results_handler'): unicode,
    Optional('name'): unicode,
    Optional('merge'): merge_schema,
    Optional('non_precommit_args'): [unicode],
    Optional('precommit_args'): [unicode],
    Optional('script'): unicode,
    Optional('swarming'): swarming_schema,
})

junit_schema = Schema({
    Optional('name'): unicode,
    'test': unicode,
})

script_schema = Schema({
    Optional('args'): [unicode],
    'name': unicode,
    'script': unicode,
})

builder_schema = Schema({
  Optional('additional_compile_targets'): [unicode],
  Optional('disabled_tests'): {unicode: unicode},
  Optional('gtest_tests'): [Or(unicode, gtest_schema)],
  Optional('isolated_scripts'): [isolated_script_schema],
  Optional('instrumentation_tests'): [instrumentation_test_schema],
  Optional('junit_tests'): [junit_schema],
  Optional('scripts'): [script_schema],
})

master_schema = Schema({Optional(unicode): builder_schema})

def main():
  os.chdir(os.path.dirname(__file__))
  for filename in glob.glob('*.json'):
    if filename in ('chromium.angle.json', 'trybot_analyze_config.json'):
      continue

    try:
      testing_json = master_schema.validate(json.load(open(filename)))
    except Exception as e:
      print 'Failed to validate "%s":\n%s' % (filename, e)
      return 1

  return 0


if __name__ == '__main__':
  sys.exit(main())

