#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Script to check validity of StoryExpectations."""

import argparse
import json
import os
import pprint

from core import benchmark_finders
from core import path_util
path_util.AddTelemetryToPath()
path_util.AddAndroidPylibToPath()

from telemetry.internal.browser import browser_options


CLUSTER_TELEMETRY_DIR = os.path.join(
    path_util.GetChromiumSrcDir(), 'tools', 'perf', 'contrib',
    'cluster_telemetry')
CLUSTER_TELEMETRY_BENCHMARKS = [
    ct_benchmark.Name() for ct_benchmark in
    benchmark_finders.GetBenchmarksInSubDirectory(CLUSTER_TELEMETRY_DIR)
]


def validate_story_names(benchmarks):
  for benchmark in benchmarks:
    if benchmark.Name() in CLUSTER_TELEMETRY_BENCHMARKS:
      continue
    b = benchmark()
    options = browser_options.BrowserFinderOptions()
    # tabset_repeat is needed for tab_switcher benchmarks.
    options.tabset_repeat = 1
    # test_path required for blink_perf benchmark in contrib/.
    options.test_path = ''
    # shared_prefs_file required for benchmarks in contrib/vr_benchmarks/
    options.shared_prefs_file = ''
    story_set = b.CreateStorySet(options)
    failed_stories = b.GetBrokenExpectations(story_set)
    assert not failed_stories, 'Incorrect story names: %s' % str(failed_stories)


def list_disables(benchmarks, path):
  # Creates a dictionary of the format:
  # {'benchmark_name1' : {
  #     'Condition Name 1': [story1, story2],
  #     'Condition Name 2': [story2, story3]
  #     }
  #  'benchmark_name2': ....
  # }
  disables = {}
  for benchmark in benchmarks:
    name = benchmark.Name()
    disables[name] = {}
    expectations = benchmark().GetExpectations().disabled_stories
    for story in expectations:
      for conditions, _ in  expectations[story]:
        for condition in conditions:
          if not disables[name].get(str(condition)):
            disables[name][str(condition)] = []
          disables[name][str(condition)].append(story)
  if path:
    print 'Disabled tests information saved to %s' % path
    with open(path, 'w') as f:
      json.dump(disables, f)
  else:
    pp = pprint.PrettyPrinter()
    pp.pprint(disables)


def main(args):
  parser = argparse.ArgumentParser(
      description=('Tests if disabled stories exist.'))
  parser.add_argument(
      '--list', action='store_true', default=False,
      help=('Prints list of disabled stories.'))
  parser.add_argument(
      '--output-file',
       help='If --list is set, this will be the file the json is saved to.'
  )
  options = parser.parse_args(args)

  benchmarks = benchmark_finders.GetAllBenchmarks()
  if options.list:
    list_disables(benchmarks, options.output_file)
  else:
    validate_story_names(benchmarks)
  return 0
