#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Custom swarming triggering script.

This script does custom swarming triggering logic, to enable device affinity
for our bots, while lumping all trigger calls under one buildbot step.

This script must have roughly the same command line interface as swarming.py
trigger. It modifies it in the following ways:
  * Inserts `id: buildXX` dimensions per shard. It does this one per the
    --bot-id arg.
  * Modifies the --dump-json argument; it inserts its own arg, and outputs a
    merged json file. This json file will have only a 'tasks' entry, which will
    have a list of all tasks triggered, with appropriate shard indices.
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile

def subprocess_call(script, args):
  return subprocess.check_call(sys.executable, script, args)

def modify_args(all_args, bot_id, temp_file):
  assert '--' in all_args, (
      'Malformed trigger command; -- argument expected but not found')
  dash_ind = all_args.index('--')

  return all_args[:dash_ind] + [
      '--dump_json', temp_file, '--dimension', 'id', bot_id] + all_args[
          dash_ind:]

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--bot-id', action='append', required=True)
  parser.add_argument('--swarming-py-path', required=True)
  args, remaining = parser.parse_known_args()

  dump_index = remaining.index('--dump-json')
  original_json_dump_path = remaining[dump_index+1]
  remaining = remaining[:dump_index] + remaining[dump_index+2:]
  merged_json = {'tasks': {}}

  # TODO: Do these in parallel
  for shard_num, bot_id in enumerate(args.bot_id):
    _, json_temp = tempfile.mkstemp(prefix='perf_device_trigger')
    try:
      args_to_pass = modify_args(remaining[:], bot_id, json_temp)

      subprocess.check_call(
          sys.executable, [args.swarming_py_path] + args_to_pass)
      with open(json_temp) as f:
        result_json = json.load(f)

      for k, v in result_json['tasks'].items():
        v['shard_index'] = shard_num
        merged_json['tasks'][k + ':%d:%d' % (shard_num, len(args.bot_id))] = v
    finally:
      os.unlink(json_temp)
  with open(original_json_dump_path, 'w') as f:
    json.dump(merged_json, f)
  return 0

if __name__ == '__main__':
  sys.exit(main())
