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

from core import path_util
from core import swarming_test_triggerer


class PerfDeviceTrigger(
    swarming_test_triggerer.SwarmingTestTriggerer):
  def __init__(self, file_prefix):
    super(PerfDeviceTrigger, self, file_prefix).__init__()

  def get_swarming_py_path():
    return os.path.join(
      path_util.GetChromiumSrcDir(), 'tools', 'swarming_client', 'swarming.py')

  def modify_args(
      self, all_args, gpu_index, shard_index, total_shards, temp_file):
    modified_args = super(PerfDeviceTrigger, self).modify_args(
        all_args, gpu_index, shard_index, total_shards, temp_file)
    # Find the id to trigger the job to and append to the list to
    # be passed to the isolated script on the bot swarming bot running
    # the test
    if 'id' in modified_args:
      modified_args + ['--bot', modified_args['id']]


  def select_config_indices(self, args, verbose):
    # For perf we want to trigger a job for every valid config since
    # each config represents exactly on bot in the perf swarming pool,
    selected_configs = []
    for i in xrange(args.shards):
      selected_configs.append(i)
    return selected_configs


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--bot-id', action='append', required=True,
                      help='Which bot IDs to trigger tasks on. Number of bot'
                      ' IDs must match the number of shards. This is because'
                      ' the recipe code uses the --shard argument to determine'
                      ' how many shards it expects in the output json.')
  parser.add_argument('--dump-json', required=True,
                      help='(Swarming Trigger Script API) Where to dump the'
                      ' resulting json which indicates which tasks were'
                      ' triggered for which shards.')
  parser.add_argument('--shards', type=int, default=1,
                      help='How many shards to trigger. Duplicated from the'                                      ' `swarming.py trigger` command.')
  parser.add_argument('--trigger-script-verbose', type=bool, default=False,
                      help='Turn on verbose logging')

  args, remaining = parser.parse_known_args()

  return PerfDeviceTrigger('trigger_perf_test').trigger_tasks(args, remaining)


if __name__ == '__main__':
  sys.exit(main())
