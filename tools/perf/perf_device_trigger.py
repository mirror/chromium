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
import copy
import json
import os
import random
import subprocess
import sys
import tempfile
import urllib

from core import path_util

SRC_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(
  os.path.abspath(__file__)))))

SWARMING_PY = os.path.join(SRC_DIR, 'tools', 'swarming_client', 'swarming.py')

def get_swarming_py_path():
  return os.path.join(
      path_util.GetChromiumSrcDir(), 'tools', 'swarming_client', 'swarming.py')

def strip_unicode(obj):
  """Recursively re-encodes strings as utf-8 inside |obj|. Returns the result.
  """
  if isinstance(obj, unicode):
    return obj.encode('utf-8', 'replace')

  if isinstance(obj, list):
    return list(map(strip_unicode, obj))

  if isinstance(obj, dict):
    new_obj = type(obj)(
        (strip_unicode(k), strip_unicode(v)) for k, v in obj.iteritems() )
    return new_obj

  return obj


class GpuTestTriggerer(object):
  def __init__(self):
    self._gpu_configs = None
    self._bot_statuses = []
    self._total_bots = 0

  def filter_swarming_py_path_arg(self, args):
    # TODO(kbr): remove this once the --swarming-py-path argument is
    # no longer being passed by recipes. crbug.com/801346
    try:
      index = args.index('--swarming-py-path')
      return args[:index] + args[index+2:]
    except:
      return args

  def modify_args(self, all_args, gpu_index, shard_index, total_shards,
                  temp_file):
    """Modifies the given argument list.

    Specifically, it does the following:
      * Adds a --dump_json argument, to read in the results of the
        individual trigger command.
      * Adds the dimensions associated with the GPU config at the given index.
      * If the number of shards is greater than one, adds --env
        arguments to set the GTEST_SHARD_INDEX and GTEST_TOTAL_SHARDS
        environment variables to _shard_index_ and _total_shards_,
        respectively.

    The arguments are structured like this:
    <args to swarming.py trigger> -- <args to bot running isolate>
    This means we have to add arguments to specific locations in the argument
    list, to either affect the trigger command, or what the bot runs.

    """
    assert '--' in all_args, (
        'Malformed trigger command; -- argument expected but not found')
    dash_ind = all_args.index('--')
    gpu_args = ['--dump-json', temp_file]
    if total_shards > 1:
      gpu_args.append('--env')
      gpu_args.append('GTEST_SHARD_INDEX')
      gpu_args.append(str(shard_index))
      gpu_args.append('--env')
      gpu_args.append('GTEST_TOTAL_SHARDS')
      gpu_args.append(str(total_shards))
    # Need to pull out the id key
    id_val = None
    for key, val in sorted(self._gpu_configs[gpu_index].iteritems()):
      if key == 'id':
        id_val = val
      gpu_args.append('--dimension')
      gpu_args.append(key)
      gpu_args.append(val)
    return all_args[:dash_ind] + gpu_args + all_args[dash_ind:] + ['--bot', id_val]

  def parse_gpu_configs(self, args):
    try:
      self._gpu_configs = strip_unicode(json.loads(args.trigger_configs))
    except Exception as e:
      raise ValueError('Error while parsing JSON from gpu config string %s: %s'
                       % (args.trigger_configs, str(e)))
    # Validate the input.
    if not isinstance(self._gpu_configs, list):
      raise ValueError('GPU configurations must be a list, were: %s' %
                       args.trigger_configs)
    if len(self._gpu_configs) < 1:
      raise ValueError('GPU configuration list must have at least one entry')
    if not all(isinstance(entry, dict) for entry in self._gpu_configs):
      raise ValueError('GPU configurations must all be dictionaries')

  def remove_swarming_dimension(self, args, dimension):
    for i in xrange(len(args)):
      if args[i] == '--dimension' and args[i+1] == dimension:
        return args[:i] + args[i+3:]
    return args

  def choose_random_int(self, max_num):
    return random.randint(1, max_num)

  def pick_gpu_configuration(self, index):
    return index

  def make_temp_file(self, prefix=None, suffix=None):
    # This trick of closing the file handle is needed on Windows in order to
    # make the file writeable.
    h, temp_file = tempfile.mkstemp(prefix=prefix, suffix=suffix)
    os.close(h)
    return temp_file

  def delete_temp_file(self, temp_file):
    os.remove(temp_file)

  def read_json_from_temp_file(self, temp_file):
    with open(temp_file) as f:
      return json.load(f)

  def write_json_to_file(self, merged_json, output_file):
    with open(output_file, 'w') as f:
      json.dump(merged_json, f)

  def run_swarming(self, args):
    return subprocess.call([sys.executable, get_swarming_py_path()] + args)

  def trigger_tasks(self, args, remaining):
    """Triggers tasks for each bot.

    Args:
      args: Parsed arguments which we need to use.
      remaining: The remainder of the arguments, which should be passed to
                 swarming.py calls.

    Returns:
      Exit code for the script.
    """
    self.parse_gpu_configs(args)

    # In the remaining arguments, find the Swarming dimensions that are
    # specified by the GPU configs and remove them, because for each shard,
    # we're going to select one of the GPU configs and put all of its Swarming
    # dimensions on the command line.
    filtered_remaining_args = copy.deepcopy(remaining)
    for config in self._gpu_configs:
      for k in config.iterkeys():
        filtered_remaining_args = self.remove_swarming_dimension(
          filtered_remaining_args, k)

    merged_json = {}

    for i in xrange(args.shards):
      # For each shard that we're going to distribute, do the following:
      # 1. Pick which GPU configuration to use.
      # 2. Insert that GPU configuration's dimensions as command line
      #    arguments, and invoke "swarming.py trigger".
      gpu_index = self.pick_gpu_configuration(i)
      # Holds the results of the swarming.py trigger call.
      try:
        json_temp = self.make_temp_file(prefix='trigger_gpu_test',
                                        suffix='.json')
        args_to_pass = self.modify_args(filtered_remaining_args, gpu_index, i,
                                        args.shards, json_temp)
        print 'invoking swarming task on shard %d' % i
        ret = self.run_swarming(args_to_pass)
        if ret:
          sys.stderr.write('Failed to trigger a task, aborting\n')
          return ret
        result_json = self.read_json_from_temp_file(json_temp)
        if i == 0:
          # Copy the entire JSON -- in particular, the "request"
          # dictionary -- from shard 0. "swarming.py collect" uses
          # some keys from this dictionary, in particular related to
          # expiration. It also contains useful debugging information.
          merged_json = copy.deepcopy(result_json)
          # However, reset the "tasks" entry to an empty dictionary,
          # which will be handled specially.
          merged_json['tasks'] = {}
        for k, v in result_json['tasks'].items():
          v['shard_index'] = i
          merged_json['tasks'][k + ':%d:%d' % (i, args.shards)] = v
      finally:
        self.delete_temp_file(json_temp)
    self.write_json_to_file(merged_json, args.dump_json)
    return 0


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--trigger-configs', type=str, required=True,
                      help='The GPU configurations to trigger tasks on, in the'
                      ' form of a JSON array of dictionaries. At least one'
                      ' entry in this dictionary is required.')
  parser.add_argument('--dump-json', required=True,
                      help='(Swarming Trigger Script API) Where to dump the'
                      ' resulting json which indicates which tasks were'
                      ' triggered for which shards.')
  parser.add_argument('--shards', type=int, default=2,
                      help='How many shards to trigger. Duplicated from the'
                      ' `swarming.py trigger` command.')

  args, remaining = parser.parse_known_args()

  return GpuTestTriggerer().trigger_tasks(args, remaining)


if __name__ == '__main__':
  sys.exit(main())
