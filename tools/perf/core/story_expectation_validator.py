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


def ListDisabledStories(benchmarks, path):
  # Creates a dictionary of the format:
  # {
  #   'benchmark_name1' : {
  #     'story_1': [
  #       {'conditions': conditions, 'reason': reason},
  #       ...
  #     ],
  #     ...
  #   },
  #   ...
  # }
  disables = {}
  for benchmark in benchmarks:
    name = benchmark.Name()
    disables[name] = {}
    expectations = benchmark().GetExpectations().AsDict()['stories']
    for story in expectations:
      for conditions, reason in  expectations[story]:
        if not disables[name].get(story):
          disables[name][story] = []
          conditions_str = [str(a) for a in conditions]
        disables[name][story].append((conditions_str, reason))
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
    ListDisabledStories(benchmarks, options.output_file)
  else:
    validate_story_names(benchmarks)
  return 0
