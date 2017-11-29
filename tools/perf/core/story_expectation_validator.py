#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Script to check validity of StoryExpectations."""

import argparse
import json
import os

from core import benchmark_finders
from core import path_util
path_util.AddTelemetryToPath()
path_util.AddAndroidPylibToPath()


from telemetry import decorators
from telemetry.internal.browser import browser_options


CLUSTER_TELEMETRY_DIR = os.path.join(
    path_util.GetChromiumSrcDir(), 'tools', 'perf', 'contrib',
    'cluster_telemetry')
CLUSTER_TELEMETRY_BENCHMARKS = [
    ct_benchmark.Name() for ct_benchmark in
    benchmark_finders.GetBenchmarksInSubDirectory(CLUSTER_TELEMETRY_DIR)
]


# TODO(rnephew): Remove this check when it is the norm to not use decorators.
def check_decorators(benchmarks):
  for benchmark in benchmarks:
    if (decorators.GetDisabledAttributes(benchmark) or
        decorators.GetEnabledAttributes(benchmark)):
      raise Exception(
          'Disabling or enabling telemetry benchmark with decorator detected. '
          'Please use StoryExpectations instead. Contact rnephew@ for more '
          'information. \nBenchmark: %s' % benchmark.Name())


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


# TODO(rnephew): This needs to be changed when we know the final version of SoM
# 1-click disabling expectations.
def _ConditionsToString(conditions):
  return ''.join([str(a).replace(' ', '_') for a in conditions])


def GetDisabledStories(benchmarks):
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
      for conditions, reason in expectations[story]:
        if not disables[name].get(story):
          disables[name][story] = []
          conditions_str = _ConditionsToString(conditions)
        disables[name][story].append((conditions_str, reason))
  return disables


# TODO(rnephew): Delete this when SoM conversion is complete.
def ConvertStories(benchmarks, file_path):
  """Writes StoryExpectations to a text file in a format compatible with SoM."""
  with open(file_path, 'w') as fp:
    for benchmark in benchmarks:
      name = benchmark.Name()
      expectations = benchmark().GetExpectations().AsDict()['stories']
      if expectations:
        fp.write('# Benchmark: %s\n' % name)
        for story in expectations:
          for conditions, reason in expectations[story]:
            c = _ConditionsToString(conditions)
            e = ''
            if reason.startswith('crbug') or reason.startswith('Bug('):
              e += '%s ' % reason
            e += '[ %s ] %s/%s [ Skip ]\n' % (c, name, story)
            fp.write(e)
        fp.write('\n')


def main(args):
  parser = argparse.ArgumentParser(
      description=('Tests if disabled stories exist.'))
  parser.add_argument(
      '--list', action='store_true', default=False,
      help=('Prints list of disabled stories.'))
  # TODO(rnephew): This option is for our conversion from StoryExpectations
  # to SoM's 1-click test disabling. It should be removed when that is done.
  parser.add_argument('--convert',
                      help=('Converts from code based expectations to a flat'
                            ' file. Pass filename to save as.'))
  options = parser.parse_args(args)
  benchmarks = benchmark_finders.GetAllBenchmarks()

  # TODO(rnephew): Delete this after conversion.
  if options.convert:
    ConvertStories(benchmarks, options.convert)
  elif options.list:
    stories = GetDisabledStories(benchmarks)
    print json.dumps(stories, sort_keys=True, indent=4, separators=(',', ': '))
  else:
    validate_story_names(benchmarks)
    check_decorators(benchmarks)
  return 0
