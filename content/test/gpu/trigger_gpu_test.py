#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
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


def modify_args(all_args, bot_id, temp_file):
  """Modifies the given argument list.

  Specifically, it does the following:
    * Adds a --dump_json argument, to read in the results of the
      individual trigger command.
    * Adds a bot id dimension.
    * Adds the bot id as an argument to the test, so it knows which tests to
      run.

  The arguments are structured like this:
  <args to swarming.py trigger> -- <args to bot running isolate>
  This means we have to add arguments to specific locations in the argument
  list, to either affect the trigger command, or what the bot runs.
  """
  assert '--' in all_args, (
      'Malformed trigger command; -- argument expected but not found')
  dash_ind = all_args.index('--')

  return all_args[:dash_ind] + [
      '--dump-json', temp_file, '--dimension', 'id', bot_id] + all_args[
          dash_ind:] + ['--id', bot_id]


def trigger_tasks(args, remaining):
  """Triggers tasks for each bot.

  Args:
    args: Parsed arguments which we need to use.
    remaining: The remainder of the arguments, which should be passed to
               swarming.py calls.

  Returns:
    Exit code for the script.
  """
  gpu_configs = None
  try:
    gpu_configs = strip_unicode(json.loads(args.gpu_trigger_configs))
  except Exception as e:
    raise Exception('Error while parsing JSON from gpu config string %s: %s' %
                    config_string, str(e))
  # Validate the input.
  if not isinstance(gpu_configs, list):
    raise Exception('GPU configurations must be a list, were: %s' %
                    args.gpu_trigger_configs)
  if len(gpu_configs) < 1:
    raise Exception('GPU configuration list must have at least one entry')
  if not all(isinstance(entry, dict) for entry in gpu_configs):
    raise Exception('GPU configurations must all be dictionaries')
  print 'Configurations:'
  print str(gpu_configs)

  print 'urlencoded arguments:'
  for config in gpu_configs:
    values = []
    for key, value in sorted(config.iteritems()):
      values.append(('dimensions', '%s:%s' % (key, value)))
    print urllib.urlencode(values)

  # Query Swarming to figure out which bots are available.
  bot_statuses = []
  for config in gpu_configs:
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
      bot_statuses.append({'total': count, 'available': available})
    finally:
      os.remove(temp_file)

  for i in xrange(len(gpu_configs)):
    print 'configuration: %s' % gpu_configs[i]
    print 'total bots: %d' % bot_statuses[i]['total']
    print 'available bots: %d' % bot_statuses[i]['available']

  sys.exit(1)


  merged_json = {'tasks': {}}

  # TODO: Do these in parallel
  for shard_num, bot_id in enumerate(args.bot_id):
    # Holds the results of the swarming.py trigger call.
    temp_fd, json_temp = tempfile.mkstemp(prefix='perf_device_trigger')
    try:
      args_to_pass = modify_args(remaining[:], bot_id, json_temp)

      ret = subprocess.call([SWARMING_PY] + args_to_pass)
      if ret:
        sys.stderr.write('Failed to trigger a task, aborting\n')
        return ret
      with open(json_temp) as f:
        result_json = json.load(f)

      for k, v in result_json['tasks'].items():
        v['shard_index'] = shard_num
        merged_json['tasks'][k + ':%d:%d' % (shard_num, len(args.bot_id))] = v
    finally:
      os.close(temp_fd)
      os.remove(json_temp)
  with open(args.dump_json, 'w') as f:
    json.dump(merged_json, f)
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

  return trigger_tasks(args, remaining)


if __name__ == '__main__':
  sys.exit(main())
