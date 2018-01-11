#!/usr/bin/env python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Custom swarming triggering script.

This script does custom swarming triggering logic, to allow one GPU bot to
conceptually span multiple Swarming configurations, while lumping all trigger
calls under one logical step.

The reason this script is needed is to allow seamless upgrades of the GPU, OS
version, or graphics driver. GPU tests are triggered with precise values for all
of these Swarming dimensions. This ensures that if a machine is added to the
Swarming pool with a slightly different configuration, tests don't fail for
unexpected reasons.

During an upgrade of the fleet, it's not feasible to take half of the machines
offline. We had some experience with this during a recent upgrade of the GPUs in
the main Windows and Linux NVIDIA bots. In the middle of the upgrade, only 50%
of the capacity was available, and CQ jobs started to time out. Once the hurdle
had been passed in the middle of the upgrade, capacity was sufficient, but it's
crucial that this process remain seamless.

This script receives multiple machine configurations on the command line in the
form of quoted strings. These strings are JSON dictionaries that represent
entries in the "dimensions" array of the "swarming" dictionary in the
src/testing/buildbot JSON files. The script queries the Swarming pool for the
number of machines of each configuration, and distributes work (shards) among
them using the following algorithm:

1. If either configuration has machines available (online, not busy at the time
of the query) then distribute shards to them first.

2. Compute the relative fractions of all of the live (online, not quarantined,
not dead) machines of all configurations.

3. Distribute the remaining shards probabilistically among these configurations.

The use of random numbers attempts to avoid the pathology where one
configuration only has a couple of machines, and work is never distributed to it
once all machines are busy.

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
import subprocess
import sys
import tempfile
import urllib


SRC_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(
  os.path.abspath(__file__)))))

SWARMING_PY = os.path.join(SRC_DIR, 'tools', 'swarming_client', 'swarming.py')


def strip_unicode(obj):
  """Recursively re-encodes strings as utf-8 inside |obj|. Returns the result.
  """
  if isinstance(obj, unicode):
    return obj.encode('utf-8', 'replace')

  if isinstance(obj, list):
    return map(strip_unicode, obj)

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

  def modify_args(self, all_args, gpu_index, temp_file):
    """Modifies the given argument list.

    Specifically, it does the following:
      * Adds a --dump_json argument, to read in the results of the
        individual trigger command.
      * Adds the dimensions associated with the GPU config at the given index.

    The arguments are structured like this:
    <args to swarming.py trigger> -- <args to bot running isolate>
    This means we have to add arguments to specific locations in the argument
    list, to either affect the trigger command, or what the bot runs.
    """
    assert '--' in all_args, (
        'Malformed trigger command; -- argument expected but not found')
    dash_ind = all_args.index('--')
    gpu_args = ['--dump-json', temp_file]
    for key, val in sorted(self._gpu_configs[gpu_index].iteritems()):
      gpu_args.append('--dimension')
      gpu_args.append(key)
      gpu_args.append(val)
    return all_args[:dash_ind] + gpu_args + all_args[dash_ind:]

  def parse_gpu_configs(self, args):
    try:
      self._gpu_configs = strip_unicode(json.loads(args.gpu_trigger_configs))
    except Exception as e:
      raise Exception('Error while parsing JSON from gpu config string %s: %s' %
                      config_string, str(e))
    # Validate the input.
    if not isinstance(self._gpu_configs, list):
      raise Exception('GPU configurations must be a list, were: %s' %
                      args.gpu_trigger_configs)
    if len(self._gpu_configs) < 1:
      raise Exception('GPU configuration list must have at least one entry')
    if not all(isinstance(entry, dict) for entry in self._gpu_configs):
      raise Exception('GPU configurations must all be dictionaries')
    print 'Configurations:'
    print str(self._gpu_configs)

  def query_swarming_for_gpu_configs(self):
    # Query Swarming to figure out which bots are available.
    for config in self._gpu_configs:
      values = []
      for key, value in sorted(config.iteritems()):
        values.append(('dimensions', '%s:%s' % (key, value)))
      # Ignore dead and quarantined bots.
      values.append(('is_dead', 'FALSE'))
      values.append(('quarantined', 'FALSE'))
      query_arg = urllib.urlencode(values)

      # This trick of closing the file handle is needed on Windows in
      # order to make the file writeable.
      h, temp_file = tempfile.mkstemp(prefix='trigger_gpu_test', suffix='.json')
      os.close(h)
      try:
        ret = subprocess.call([SWARMING_PY,
                               'query',
                               '-S',
                               'chromium-swarm.appspot.com',
                               '--json',
                               temp_file,
                               ('bots/count?%s' % query_arg)])
        if ret:
          raise Exception('Error running swarming.py')
        with open(temp_file) as fp:
          query_result = strip_unicode(json.load(fp))
        # Summarize number of available bots per configuration.
        count = int(query_result['count'])
        # Be robust against errors in computation.
        available = max(0, count - int(query_result['busy']))
        self._bot_statuses.append({'total': count, 'available': available})
      finally:
        os.remove(temp_file)
    # Sum up the total count of all bots.
    self._total_bots = sum(x['total'] for x in self._bot_statuses)

  def remove_swarming_dimension(self, args, dimension):
    for i in xrange(len(args)):
      if args[i] == '--dimension' and args[i+1] == dimension:
        return args[:i] + args[i+3:]
    return args

  def choose_random_int(self, max_num):
    return random.randint(1, max_num)

  def pick_gpu_configuration(self):
    # These are the rules used:
    # 1. If any configuration has bots available, pick the configuration with
    #    the most bots available.
    # 2. If no configuration has bots available, pick a random configuration
    #    based on the total number of bots in each configuration.
    #
    # This method updates bot_statuses_ in case (1), and in both cases, returns
    # the index into gpu_configs_ that should be used.
    if any(status['available'] > 0 for status in self._bot_statuses):
      # Case 1.
      max_index = 0
      max_val = self._bot_statuses[0]['available']
      for i in xrange(1, len(self._bot_statuses)):
        avail = self._bot_statuses[i]['available']
        if avail > max_val:
          max_index = i
          max_val = avail
      self._bot_statuses[max_index]['available'] -= 1
      assert self._bot_statuses >= 0
      return max_index
    # Case 2.
    r = self.choose_random_int(self._total_bots)
    for i in xrange(0, len(self._bot_statuses)):
      status = self._bot_statuses[i]
      if r <= status['total']:
        return i
      r -= status['total']
    raise Exception('Should not reach here')

  def make_temp_file(self, prefix=None, suffix=None):
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
    return subprocess.call([sys.executable, SWARMING_PY] + args)

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
    self.query_swarming_for_gpu_configs()

    # In the remaining arguments, find the Swarming dimensions that are
    # specified by the GPU configs and remove them, because for each shard,
    # we're going to select one of the GPU configs and put all of its Swarming
    # dimensions on the command line.
    filtered_remaining_args = copy.deepcopy(remaining)
    for config in self._gpu_configs:
      for k in config.iterkeys():
        filtered_remaining_args = self.remove_swarming_dimension(
          filtered_remaining_args, k)

    merged_json = {'tasks': {}}

    for i in xrange(args.shards):
      # For each shard that we're going to distribute, do the following:
      # 1. Pick which GPU configuration to use.
      # 2. Insert that GPU configuration's dimensions as command line
      #    arguments, and invoke "swarming.py trigger".
      gpu_index = self.pick_gpu_configuration()
      # Holds the results of the swarming.py trigger call.
      try:
        json_temp = self.make_temp_file(prefix='trigger_gpu_test',
                                        suffix='.json')
        args_to_pass = self.modify_args(filtered_remaining_args,
                                        gpu_index, json_temp)
        ret = self.run_swarming(args_to_pass)
        if ret:
          sys.stderr.write('Failed to trigger a task, aborting\n')
          return ret
        result_json = self.read_json_from_temp_file(json_temp)
        for k, v in result_json['tasks'].items():
          v['shard_index'] = i
          merged_json['tasks'][k + ':%d:%d' % (i, args.shards)] = v
      finally:
        self.delete_temp_file(json_temp)
    self.write_json_to_file(merged_json, args.dump_json)
    return 0


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--gpu-trigger-configs', type=str, required=True,
                      help='The GPU configurations to trigger tasks on, in the'
                      ' form of a JSON array of dictionaries. At least one'
                      ' entry in this dictionary is required.')
  parser.add_argument('--dump-json', required=True,
                      help='(Swarming Trigger Script API) Where to dump the'
                      ' resulting json which indicates which tasks were'
                      ' triggered for which shards.')
  parser.add_argument('--shards', type=int, default=1,
                      help='How many shards to trigger. Duplicated from the'
                      ' `swarming.py trigger` command.')
  args, remaining = parser.parse_known_args()

  return GpuTestTriggerer().trigger_tasks(args, remaining)


if __name__ == '__main__':
  sys.exit(main())
