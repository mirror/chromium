# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to generate benchmark_sharding_map.json in the //tools/perf/core
directory. This file controls which bots run which tests.

The file is a JSON dictionary. It maps waterfall name to a mapping of benchmark
to bot id. E.g.

{
  "build1-b1": {
    "benchmarks": [
      "battor.steady_state",
      ...
    ],
  }
}

This will be used to manually shard tests to certain bots, to more efficiently
execute all our tests.
"""

import argparse
import collections
import itertools
import json
import os
import subprocess

from core import path_util
path_util.AddTelemetryToPath()

from telemetry.util import bot_utils

from telemetry import decorators

def get_sharding_map_path():
  return os.path.join(
      path_util.GetChromiumSrcDir(), 'tools', 'perf', 'core',
      'benchmark_sharding_map.json')

def get_avg_times_path():
  return os.path.join(
      path_util.GetChromiumSrcDir(), 'tools', 'perf', 'core',
      'desktop_benchmark_avg_times.json')

def load_benchmark_sharding_map():
  with open(get_sharding_map_path()) as f:
    raw = json.load(f)

  # The raw json format is easy for people to modify, but isn't what we want
  # here. Change it to map builder -> benchmark -> device.
  final_map = {}
  for builder, builder_map in raw.items():
    if builder == 'all_benchmarks':
      continue

    final_builder_map = {}
    for device, device_value in builder_map.items():
      for benchmark_name in device_value['benchmarks']:
        final_builder_map[benchmark_name] = device
    final_map[builder] = final_builder_map

  return final_map


# Returns a sorted list of (benchmark, avg_runtime) pairs for every
# benchmark in the all_benchmarks list where avg_runtime is in seconds.  Also
# returns a list of benchmarks whose run time have not been seen before
def get_sorted_benchmark_list_by_time(all_benchmarks):
  runtime_list = []
  benchmark_avgs = {}
  new_benchmarks = []
  timing_file_path = os.path.join(
      path_util.GetChromiumSrcDir(), 'tools', 'perf', 'core',
      'desktop_benchmark_avg_times.json')
  # Load in the avg times as calculated on Nov 1st, 2016
  with open(timing_file_path) as f:
    benchmark_avgs = json.load(f)

  for benchmark in all_benchmarks:
    benchmark_avg_time = benchmark_avgs.get(benchmark.Name(), None)
    if benchmark_avg_time is None:
      # Assume that this is a new benchmark that was added after 11/1/16 when
      # we generated the benchmarks. Use the old affinity algorithm after
      # we have given the rest the same distribution, add it to the
      # new benchmarks list.
      new_benchmarks.append(benchmark)
    else:
      # Need to multiple the seconds by 2 since we will be generating two tests
      # for each benchmark to be run on the same shard for the reference build
      runtime_list.append((benchmark, benchmark_avg_time * 2.0))

  # Return a reverse sorted list by runtime
  runtime_list.sort(key=lambda tup: tup[1], reverse=True)
  return runtime_list, new_benchmarks


def ShouldBenchmarkBeScheduled(benchmark, platform=None):
  disabled_tags = decorators.GetDisabledAttributes(benchmark)
  enabled_tags = decorators.GetEnabledAttributes(benchmark)

  # Don't run benchmarks which are disabled on all platforms.
  if 'all' in disabled_tags:
    return False

  if platform is not None:
    # If we're not on android, don't run mobile benchmarks.
    if platform != 'android' and 'android' in enabled_tags:
      return False

    # If we're on android, don't run benchmarks disabled on mobile
    if platform == 'android' and 'android' in disabled_tags:
      return False

  return True




from core.test_timings_generator import get_story_times_path


def get_story_avg_times():
  # Format is {} benchmark -> {} story name -> time (seconds)
  with open(get_story_times_path()) as f:
    return json.load(f)


class Tmp(object):
  def __getattr__(self, name):
    return None

def shard_all_benchmarks(num_devices, all_benchmarks, builder):
  with open(get_avg_times_path()) as f:
    times = json.load(f)
  r_story_times = {}

  plat = None
  if 'win' in builder.lower():
    plat = 'win'
  elif 'mac' in builder.lower():
    plat = 'mac'
  elif 'android' in builder.lower():
    plat = 'android'
  elif 'linux' in builder.lower():
    plat = 'linux'

  story_times = get_story_avg_times()
  story_times = story_times.setdefault(plat, {}).setdefault(builder, {})

  executed_stories = []
  stories_per_benchmark = {}
  for b in all_benchmarks:
    if not (ShouldBenchmarkBeScheduled(
        b, 'android') or ShouldBenchmarkBeScheduled(b, 'win')):
      continue

    # pass in Tmp() because otherwise it breaks sometimes.
    stories = b().CreateStorySet(Tmp()).stories

    tag_filter = b.options.get('story_tag_filter')
    if tag_filter:
      stories = filter(lambda story: tag_filter in story.tags, stories)

    if b.Name() not in times:
      #print 'benchmark %s not scheduled; exiting' % b.Name()
      continue
    stories_per_benchmark[b.Name()] = [story.name for story in stories]

    for story in stories:
      story_name = story.name
      if story.grouping_keys:
        story_name += '@%s' % str(story.grouping_keys)

      tup = (b.Name(), story_name)
      time = None
      if story_times.get(b.Name()):
        benchmark_times = story_times[b.Name()]
        if not story_name in benchmark_times:
          time = 0
          continue
        time = benchmark_times[story_name]
      else:
        time = 0

      r_story_times[tup] = time
      executed_stories.append(tup)

  total_time = 0
  for story_tup in executed_stories:
    time = r_story_times[story_tup]
    if isinstance(time, list):
      time = sum(time)
    total_time += time

  chunk_size = total_time / num_devices

  sharded = []
  so_far = 0
  extra = 0
  buildup = []
  for i, name in enumerate(executed_stories):
    time = r_story_times[name]
    if isinstance(time, list):
      time = sum(time)
    if so_far + extra > chunk_size:
      print 'skipped after adding', r_story_times[executed_stories[i-1]]
      extra = (so_far + extra) - chunk_size
      sharded.append(buildup)
      buildup = []
      so_far = 0
    so_far += time
    buildup.append(name)
  def mb_sum(x):
    if isinstance(x, list):
      return sum(x)
    return x

  for i, thing in enumerate(sharded):
    continue
    print 'SHARD %d takes %d' % (i+1, sum(
        mb_sum(r_story_times[tup]) for tup in thing))
    #for tup in thing:
      #print tup[0], tup[1], r_story_times[tup]
  results = [collections.defaultdict(list) for _ in range(len(sharded))]
  for i, shard in enumerate(sharded):
    for name, story_name in shard:
      results[i][name].append(story_name)

  per_device = {}
  benchmark_end = {}
  for i, result in enumerate(results):
    per_device[i] = result
    for name in sorted(result.keys()):
      if len(result[name]) == len(stories_per_benchmark[name]):
        result[name] = {}
      else:
        val = benchmark_end.get(name, -1) + 1
        result[name] = {
            "begin": val,
            "end": len(result[name]) + val,
        }
        benchmark_end[name] = result[name]["end"]

  return results


# Returns a map of benchmark name to shard it is on.
def shard_benchmarks(num_shards, all_benchmarks):
  benchmark_to_shard_dict = {}
  shard_execution_times = [0] * num_shards
  sorted_benchmark_list, new_benchmarks = get_sorted_benchmark_list_by_time(
    all_benchmarks)
  # Iterate over in reverse order and add them to the current smallest bucket.
  for benchmark in sorted_benchmark_list:
    # Find current smallest bucket
    min_index = shard_execution_times.index(min(shard_execution_times))
    benchmark_to_shard_dict[benchmark[0].Name()] = min_index
    shard_execution_times[min_index] += benchmark[1]
  # For all the benchmarks that didn't have avg run times, use the default
  # device affinity algorithm
  for benchmark in new_benchmarks:
     device_affinity = bot_utils.GetDeviceAffinity(num_shards, benchmark.Name())
     benchmark_to_shard_dict[benchmark.Name()] = device_affinity
  return benchmark_to_shard_dict

def regenerate(
    benchmarks, waterfall_configs, dry_run, verbose, builder_names=None):
  """Regenerate the shard mapping file.

  This overwrites the current file with fresh data.
  """
  if not builder_names:
    builder_names = []

  with open(get_sharding_map_path()) as f:
    sharding_map = json.load(f)
  sharding_map[u'all_benchmarks'] = [b.Name() for b in benchmarks]

  for name, config in waterfall_configs.items():
    if 'fyi' in name:
      continue
    for builder, tester in config['testers'].items():
      if not tester.get('swarming'):
        continue

      if builder_names and builder not in builder_names:
        continue
      per_builder = {}

      devices = sorted(tester['swarming_dimensions'][0]['device_ids'])
      shard_number = len(devices)
      shards = shard_all_benchmarks(shard_number, benchmarks, builder)

      for index, shard in enumerate(shards):
        device = devices[index]
        device_map = per_builder.get(device, {'benchmarks': {}})
        for benchmark, value in shard.items():
          device_map['benchmarks'][benchmark] = value
        per_builder[device] = device_map

      sharding_map[builder] = per_builder


  for name, builder_values in sharding_map.items():
    if name == 'all_benchmarks':
      builder_values.sort()
      continue

  if not dry_run:
    with open(get_sharding_map_path(), 'w') as f:
      dump_json(sharding_map, f)
  else:
    f_string = 'Would have dumped new json file to %s.'
    if verbose:
      f_string += ' File contents:\n %s'
      print f_string % (get_sharding_map_path(), dumps_json(sharding_map))
    else:
      f_string += ' To see full file contents, pass in --verbose.'
      print f_string % get_sharding_map_path()

  return 0


def get_args():
  parser = argparse.ArgumentParser(
      description=('Generate perf test sharding map.'
                   'This needs to be done anytime you add/remove any existing'
                   'benchmarks in tools/perf/benchmarks.'))

  parser.add_argument(
      '--builder-names', '-b', action='append', default=None,
      help='Specifies a subset of builders which should be affected by commands'
           '. By default, commands affect all builders.')
  parser.add_argument(
      '--dry-run', action='store_true',
      help='If the current run should be a dry run. A dry run means that any'
      ' action which would be taken (write out data to a file, for example) is'
      ' instead simulated.')
  parser.add_argument(
      '--verbose', action='store_true',
      help='Determines how verbose the script is.')
  parser.add_argument('--refresh-times', action='store_true')
  parser.add_argument('--platforms', action='append', default=None)
  parser.add_argument('--swarming-py-path')
  parser.add_argument('--buildnum-offset')
  return parser


def dump_json(data, f):
  """Utility method to dump json which is indented, sorted, and readable"""
  return json.dump(data, f, indent=2, sort_keys=True, separators=(',', ': '))

def dumps_json(data):
  """Utility method to dump json which is indented, sorted, and readable"""
  return json.dumps(data, indent=2, sort_keys=True, separators=(',', ': '))

def main(args, benchmarks, configs):
  return regenerate(
      benchmarks, configs, args.dry_run, args.verbose, args.builder_names)
