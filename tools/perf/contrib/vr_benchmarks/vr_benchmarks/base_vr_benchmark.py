# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from core import perf_benchmark


class _BaseVRBenchmark(perf_benchmark.PerfBenchmark):

  @classmethod
  def AddBenchmarkCommandLineArgs(cls, parser):
    parser.add_option(
        '--shared-prefs-file',
        help='The path relative to the Chromium source root '
        'to a file containing a JSON list of shared '
        'preference files to edit and how to do so. '
        'See examples in //chrome/android/'
        'shared_preference_files/test/')
