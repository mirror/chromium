#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=too-many-lines

"""Script to check validity of StoryExpectations."""

import os

from core import path_util
path_util.AddTelemetryToPath()

from telemetry import benchmark as benchmark_module
from telemetry.core import discover
from telemetry.internal.browser import browser_options


def get_benchmark_classes():
  benchmark_dir = os.path.join(
      path_util.GetChromiumSrcDir(), 'tools', 'perf', 'benchmarks')
  top_level_dir = os.path.dirname(benchmark_dir)

  return discover.DiscoverClasses(
      benchmark_dir, top_level_dir, benchmark_module.Benchmark,
      index_by_class_name=True).values()


def validate_story_names(benchmarks):
  for benchmark in benchmarks:
    b = benchmark()
    options = browser_options.BrowserFinderOptions()
    # tabset_repeat is needed for tab_switcher benchmarks.
    options.tabset_repeat = 1
    story_set = b.CreateStorySet(options)
    # TODO(rnephew): Rename ValidateExpectations to ValidateStoryExpectations()
    failed_stories = b.ValidateExpectations(story_set)
    assert not failed_stories, 'Incorrect story names: %s' % str(failed_stories)


def main(args):
  del args  # unused
  benchmarks = get_benchmark_classes()
  validate_story_names(benchmarks)
  return 0
