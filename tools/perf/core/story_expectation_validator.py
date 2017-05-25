#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Script to check validity of StoryExpectations."""

from core import benchmark_finders
from core import path_util
path_util.AddTelemetryToPath()

from telemetry.internal.browser import browser_options


def validate_story_names(benchmarks):
  for benchmark in benchmarks:
    b = benchmark()
    options = browser_options.BrowserFinderOptions()
    # tabset_repeat is needed for tab_switcher benchmarks.
    options.tabset_repeat = 1
    story_set = b.CreateStorySet(options)
    failed_stories = b.GetBrokenExpectations(story_set)
    assert not failed_stories, 'Incorrect story names: %s' % str(failed_stories)


def main(args):
  del args  # unused
  benchmarks = benchmark_finders.GetAllPerfBenchmarks()
  validate_story_names(benchmarks)
  return 0
