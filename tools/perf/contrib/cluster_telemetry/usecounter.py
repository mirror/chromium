# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from core import perf_benchmark

from contrib.cluster_telemetry import ct_benchmarks_util
from contrib.cluster_telemetry import page_set as ct_page_set
from measurements import usecounter

class UseCounterCT(perf_benchmark.PerfBenchmark):
  """Measure use counters in cluster telemetry"""
  test = usecounter.UseCounter
  @classmethod
  def Name(cls):
    return 'usecounter_ct'

  @classmethod
  def AddBenchmarkCommandLineArgs(cls, parser):
    ct_benchmarks_util.AddBenchmarkCommandLineArgs(parser)

  @classmethod
  def ProcessCommandLineArgs(cls, parser, args):
    ct_benchmarks_util.ValidateCommandLineArgs(parser, args)

  def CreateStorySet(self, options):
    return ct_page_set.CTPageSet(
        options.urls_list, options.user_agent, options.archive_data_file)
