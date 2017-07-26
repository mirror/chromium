#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module fetches and prints the dependencies given a benchmark."""

import argparse
import optparse
import os
import sys

from core import benchmark_finders
from core import path_util

path_util.AddPyUtilsToPath()
from py_utils import cloud_storage

path_util.AddTelemetryToPath()
from telemetry import benchmark_runner

from chrome_telemetry_build import chromium_config


def _FetchDependenciesIfNeeded(story_set):
  """ Download files needed by a user story set. """
  if not story_set.wpr_archive_info:
    return

  # Download files in serving_dirs.
  serving_dirs = story_set.serving_dirs
  for directory in serving_dirs:
    cloud_storage.GetFilesInDirectoryIfChanged(directory, story_set.bucket)

  # Download WPR files.
  if any(not story.is_local for story in story_set):
    story_set.wpr_archive_info.DownloadArchivesIfNeeded()


def _EnumerateDependencies(story_set):
  """Enumerates paths of files needed by a user story set."""
  deps = set()
  # Enumerate WPRs
  for story in story_set:
    deps.add(story_set.WprFilePathForStory(story))

  # Enumerate files in serving_dirs
  for directory in story_set.serving_dirs:
    if not os.path.isdir(directory):
      raise ValueError('Must provide a valid directory.')
    # Don't allow the root directory to be a serving_dir.
    if directory == os.path.abspath(os.sep):
      raise ValueError('Trying to serve root directory from HTTP server.')
    for dirpath, _, filenames in os.walk(directory):
      for filename in filenames:
        path_name, extension = os.path.splitext(
            os.path.join(dirpath, filename))
        if extension == '.sha1':
          deps.add(path_name)

  # Return relative paths.
  prefix_len = len(os.path.realpath(path_util.GetChromiumSrcDir())) + 1
  return [dep[prefix_len:] for dep in deps if dep]


def FetchDepsForBenchmark(benchmark, output):
  # Create a dummy options object which hold default values that are expected
  # by Benchmark.CreateStorySet(options) method.
  parser = optparse.OptionParser()
  benchmark.AddBenchmarkCommandLineArgs(parser)
  options, _ = parser.parse_args([])
  story_set = benchmark().CreateStorySet(options)

  # Download files according to specified benchmark.
  _FetchDependenciesIfNeeded(story_set)

  # Print files downloaded.
  deps = _EnumerateDependencies(story_set)
  for dep in deps:
    print >> output, dep


def GetStorySet(benchmark, user_agent=None):
  # Create a dummy options object which hold default values that are expected
  # by Benchmark.CreateStorySet(options) method.
  parser = optparse.OptionParser()
  benchmark.AddBenchmarkCommandLineArgs(parser)
  options, _ = parser.parse_args([])
  options.user_agent = user_agent
  return benchmark().CreateStorySet(options)


def main(args, output):
  parser = argparse.ArgumentParser(
         description='Fetch the dependencies of perf benchmark(s).')
  parser.add_argument('benchmark_name', type=str, nargs='?')
  parser.add_argument('--force', '-f',
                      help=('Force fetching all the benchmarks when '
                            'benchmark_name is not specified'),
                      action='store_true', default=False)

  options = parser.parse_args(args)

  if options.benchmark_name:
    config = chromium_config.ChromiumConfig(
        top_level_dir=path_util.GetPerfDir(),
        benchmark_dirs=[os.path.join(path_util.GetPerfDir(), 'benchmarks')])
    benchmark = benchmark_runner.GetBenchmarkByName(
        options.benchmark_name, config)
    if not benchmark:
      raise ValueError('No such benchmark: %s' % options.benchmark_name)
    FetchDepsForBenchmark(benchmark, output)
  else:
    page_sets_affected = {}
    buckets = {}
    benchmarks_to_skip = ['skpicture_printer_ct',
                          'screenshot_ct',
                          'repaint_ct',
                          'rasterize_and_record_micro_ct',
                          'multipage_skpicture_printer_ct',
                          'loading.cluster_telemetry',
                          'skpicture_printer',
                          'cros_tab_switching.typical_24',
                          'multipage_skpicture_printer']
    for b in benchmark_finders.GetAllBenchmarks():
      if b.Name() in benchmarks_to_skip:
        print "skipping %s" % b.Name()
        continue
      print "creating %s" % b.Name()
      s = GetStorySet(b)
      if s.archive_data_file == None or s.archive_data_file == '':
        continue
      if s.bucket != None:
        if not s.archive_data_file in buckets:
          buckets[s.archive_data_file] = []
        buckets[s.archive_data_file] += [s.bucket]
      if not s.archive_data_file in page_sets_affected:
        page_sets_affected[s.archive_data_file] = []
      page_sets_affected[s.archive_data_file] += [b.Name()]
    for name, k, v in sorted([(k.split('/')[-1], k, v) for (k, v) in page_sets_affected.iteritems()]):
      bucket_name = list(set(buckets[k]))
      assert len(bucket_name) == 1
      print name, ',', (' ').join(v), ',', bucket_name[0]
      # print >> output, ('Fetch dependencies for benchmark %s' % b.Name())
      # FetchDepsForBenchmark(b, output)

if __name__ == '__main__':
  main(sys.argv[1:], sys.stdout)
