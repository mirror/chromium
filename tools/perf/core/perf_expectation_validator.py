#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
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


def current_benchmarks():
  benchmark_dir = os.path.join(
      path_util.GetChromiumSrcDir(), 'tools', 'perf', 'benchmarks')
  top_level_dir = os.path.dirname(benchmark_dir)

  return discover.DiscoverClasses(
      benchmark_dir, top_level_dir, benchmark_module.Benchmark,
      index_by_class_name=True).values()


def validate_story_name(benchmarks):
  for benchmark in benchmarks:
    b = benchmark()
    options = browser_options.BrowserFinderOptions()
    # tabset_repeat is needed for tab_switcher benchmarks.
    options.tabset_repeat = 1
    story_set = b.CreateStorySet(options)
    stories = [s.display_name for s in story_set]
    expectations = b.GetExpectations()
    # TODO(rnephew): Make it so you can access the expectations using @property
    # pylint: disable=protected-access
    for expectation in expectations._expectations:
      assert expectation in stories, ('Story name %s does not appear in '
                                      'the pageset.' % expectation)
      assert len(expectation) < 50, ('Story name must be less than 50 '
                                      'characters long')


def main(args):
  del args # unused
  benchmarks = current_benchmarks()
  validate_story_name(benchmarks)
  return 0
