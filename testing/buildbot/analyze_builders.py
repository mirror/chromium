#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Analyzes the //testing/buildbot JSON files."""

from __future__ import print_function

import argparse
import json
import os
import sys


from collections import defaultdict

import buildbot_json_validator


def key(obj):
  return json.dumps(obj, sort_keys=True, indent=2)

def key_lines(key):
  return len(key.splitlines())

def main(argv):
  args = buildbot_json_validator.parse_args(argv, __doc__)
  masters = {}
  for path in args.paths:
    master = os.path.basename(path).replace('.json', '')
    masters[master] = buildbot_json_validator.validate_file(path)

  distinct_steps = defaultdict(int)
  distinct_steps_minus_swarming = defaultdict(int)
  step_names = defaultdict(int)
  swarming_specs = defaultdict(int)
  num_masters = 0
  num_builders = 0
  num_steps = 0
  for mastername, master_data in masters.items():
    num_masters += 1
    for buildername, builder_data in master_data.items():
      num_builders += 1
      for step_type, steps in builder_data.items():
        if step_type in ('additional_compile_targets', 'disabled_tests'):
          continue
        for step in steps:
          num_steps += 1
          step_names[step.get('name', step.get('test'))] += 1
          distinct_steps[key(step)] += 1
          step_minus_swarming = step.copy()
          try:
            swarming = step_minus_swarming.pop('swarming')
            swarming_specs[key(swarming)] += 1
          except KeyError:
            pass
          distinct_steps_minus_swarming[key(step_minus_swarming)] += 1

  print('%d masters' % num_masters)
  print('%d builders' % num_builders)
  print('%d steps' % num_steps)
  print('%d distinct_steps' % len(distinct_steps))
  print('%d distinct_steps_minus_swarming' % len(distinct_steps_minus_swarming))
  print('%d step_names' % len(step_names))
  print('%d swarming_specs' % len(swarming_specs))
  print('%d lines_of_steps' % sum(map(key_lines, distinct_steps_minus_swarming.keys())))
  print('%d lines_of_swarming' % sum(map(key_lines, swarming_specs.keys())))
  print('%d lines_of_json' % key_lines(key(masters)))
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))

