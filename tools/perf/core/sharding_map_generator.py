# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to generate benchmark_sharding_map.json in the //tools/perf/core
directory. This file controls which bots run which tests.

The file is a JSON dictionary. It maps waterfall name to a mapping of benchmark
to bot id. E.g.

{
  chromium.perf: {
    battor.power: build1-b1
  }
}
"""

import json
import os

from core import path_util
path_util.AddTelemetryToPath()

from telemetry.util import bot_utils

def src_dir():
  file_path = os.path.abspath(__file__)
  return os.path.dirname(os.path.dirname(
      os.path.dirname(os.path.dirname(file_path))))

# Returns a sorted list of (benchmark, avg_runtime) pairs for every
# benchmark in the all_benchmarks list where avg_runtime is in seconds.  Also
# returns a list of benchmarks whose run time have not been seen before
def get_sorted_benchmark_list_by_time(all_benchmarks):
  runtime_list = []
  benchmark_avgs = {}
  new_benchmarks = []
  timing_file_path = os.path.join(src_dir(), 'tools', 'perf', 'core',
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

def regenerate(benchmarks, waterfall_configs):
  """Regenerate the shard mapping file.

  This overwrites the current file with fresh data.
  """
  sharding_map = {}
  for name, config in waterfall_configs.items():
    per_waterfall = {}
    sharding_map[name] = per_waterfall
    for name, tester in config['testers'].items():
      if not tester.get('swarming'):
        continue

      devices = tester['swarming_dimensions'][0]['device_ids']
      shard_number = len(devices)
      shard = shard_benchmarks(shard_number, benchmarks)
      per_waterfall[name] = {
          name: devices[index]
          for name, index in shard.items()
      }

  map_filepath = os.path.join(
      src_dir(), 'tools', 'perf', 'core', 'benchmark_sharding_map.json')
  with open(map_filepath, 'w') as f:
    json.dump(sharding_map, f, indent=2)

  return 0

def main(args, benchmarks, configs):
  if args.mode == 'regenerate':
    return regenerate(benchmarks, configs)

