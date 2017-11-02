
#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=too-many-lines

"""Script to generate chromium.perf.json and chromium.perf.fyi.json in
the src/testing/buildbot directory and benchmark.csv in the src/tools/perf
directory. Maintaining these files by hand is too unwieldy.
"""
import argparse
import collections
import csv
import json
import os
import re
import sys
import sets


from core import path_util
path_util.AddTelemetryToPath()

from telemetry import benchmark as benchmark_module
from telemetry import decorators
from telemetry.story import expectations

from py_utils import discover

from core import perf_data_generator
from core.sharding_map_generator import load_benchmark_sharding_map



def validate_benchmarks(benchmarks, benchmark_sharding_map):
  """for benchmark in benchmarks:
    # First figure out swarming dimensions this test needs to be triggered on.
    # For each set of dimensions it is only triggered on one of the devices
  swarming_dimensions = []
    for dimension in tester_config['swarming_dimensions']:
      device = None
      sharding_map = benchmark_sharding_map.get(name, None)
      device = sharding_map.get(benchmark.Name(), None)
      if not device:
        raise ValueError('No sharding map for benchmark %r found. Please '
                         'add the benchmark to '
                         '_UNSCHEDULED_TELEMETRY_BENCHMARKS list, '
                         'then file a bug with Speed>Benchmarks>Waterfall '
                         'component and assign to eyaich@ or martiniss@ to '
                         'schedule the benchmark on the perf waterfall.' % (
                             benchmark.Name()))
      swarming_dimensions.append(get_swarming_dimension(
          dimension, device))"""
  return 1


def main(args):
  parser = argparse.ArgumentParser(
      description=('Validates the sharding map is up-to-date.'))
  options = parser.parse_args(args)

  all_benchmarks = perf_data_generator.current_benchmarks()
  benchmark_sharding_map = perf_data_generator.load_benchmark_sharding_map()

  if validate_benchmarks(all_benchmarks, benchmark_sharding_map):
    print 'All the perf JSON config files are up-to-date. \\o/'
    return 0
  else:
    print ('The perf JSON config files are not up-to-date. Please run %s '
         'without --validate-only flag to update the perf JSON '
         'configs and benchmark.csv.') % sys.argv[0]
    return 1
