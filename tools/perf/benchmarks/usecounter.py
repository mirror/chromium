# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from core import perf_benchmark

from measurements import usecounter
import page_sets

class UseCounterTop10(perf_benchmark.PerfBenchmark):
  page_set = page_sets.Top10PageSet
  test = usecounter.UseCounter
  @classmethod
  def Name(cls):
    return 'usecounter.top10'

# Top2012Q3PageSet was removed in http://crrev.com/72b066
# class UseCounterTopDesktop(perf_benchmark.PerfBenchmark):
#   page_set = page_sets.Top2012Q3PageSet
#   test = usecounter.UseCounter
#   @classmethod
#   def Name(cls):
#     return 'usecounter.top_desktop'

class UseCounterTop25(perf_benchmark.PerfBenchmark):
  page_set = page_sets.Top25PageSet
  test = usecounter.UseCounter
  @classmethod
  def Name(cls):
    return 'usecounter.top25'

# Alexa1To10000PageSet was removed in http://crrev.com/72b066
# class UseCounterTop10k(perf_benchmark.PerfBenchmark):
#   page_set = page_sets.Alexa1To10000PageSet
#   test = usecounter.UseCounter
#   @classmethod
#   def Name(cls):
#     return 'usecounter.top10k'
