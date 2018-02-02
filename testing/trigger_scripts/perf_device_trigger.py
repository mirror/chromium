#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Custom swarming triggering script.

This script does custom swarming triggering logic, to enable device affinity
for our bots, while lumping all trigger calls under one logical step.

This script must have roughly the same command line interface as swarming.py
trigger. It modifies it in the following ways:
 * Inserts bot id into swarming trigger dimensions, and swarming task arguments.
 * Intercepts the dump-json argument, and creates its own by combining the
   results from each trigger call.

This script is normally called from the swarming recipe module in tools/build.
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile

import base_test_triggerer

class PerfDeviceTrigger(base_test_triggerer.BaseTestTriggerer):
  def __init__(self):
    super(PerfDeviceTrigger, self).__init__()

  def append_additional_args(self, args):
    if 'id' in args:
      # Adds the bot id as an argument to the test.
      return args + ['--bot', args['id']]
    else:
      raise Exception('Id must be present for perf device triggering')

  def select_config_indices(self, args, verbose):
    # For perf we want to trigger a job for every valid config since
    # each config represents exactly on bot in the perf swarming pool,
    selected_configs = []
    for i in xrange(args.shards):
      selected_configs.append(i)
    return selected_configs


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--multiple-trigger-configs', type=str, required=True,
                      help='The Swarming configurations to trigger tasks on, '
                      'in the form of a JSON array of dictionaries (these are '
                      'Swarming dimension_sets). At least one entry in this '
                      'dictionary is required.')
  parser.add_argument('--dump-json', required=True,
                      help='(Swarming Trigger Script API) Where to dump the'
                      ' resulting json which indicates which tasks were'
                      ' triggered for which shards.')
  parser.add_argument('--shards', type=int, default=1,
                      help='How many shards to trigger. Duplicated from the'                                      ' `swarming.py trigger` command.')
  parser.add_argument('--trigger-script-verbose', type=bool, default=False,
                      help='Turn on verbose logging')

  args, remaining = parser.parse_known_args()

  return PerfDeviceTrigger().trigger_tasks(args, remaining)

if __name__ == '__main__':
  sys.exit(main())

