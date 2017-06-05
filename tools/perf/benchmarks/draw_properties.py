# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from core import perf_benchmark

from contrib.cluster_telemetry import ct_benchmarks_util
from measurements import draw_properties
import contrib.cluster_telemetry.page_set
import page_sets
from telemetry import benchmark


class _DrawProperties(perf_benchmark.PerfBenchmark):

  @classmethod
  def Name(cls):
    return 'draw_properties'

  def CreatePageTest(self, options):
    return draw_properties.DrawProperties()

# This benchmark depends on tracing categories available in M43
# This benchmark is still useful for manual testing, but need not be enabled
# and run regularly.
@benchmark.Disabled('all')
@benchmark.Owner(emails=['enne@chromium.org'])
class DrawPropertiesToughScrolling(_DrawProperties):
  page_set = page_sets.ToughScrollingCasesPageSet

  @classmethod
  def Name(cls):
    return 'draw_properties.tough_scrolling'


# This benchmark depends on tracing categories available in M43
# This benchmark is still useful for manual testing, but need not be enabled
# and run regularly.
@benchmark.Disabled('all')
@benchmark.Owner(emails=['enne@chromium.org'])
class DrawPropertiesTop25(_DrawProperties):
  """Measures the performance of computing draw properties from property trees.

  http://www.chromium.org/developers/design-documents/rendering-benchmarks
  """
  page_set = page_sets.Top25SmoothPageSet

  @classmethod
  def Name(cls):
    return 'draw_properties.top_25'

# This benchmark depends on tracing categories available in M43
# This benchmark is still useful for manual testing, but need not be enabled
# and run regularly.
@benchmark.Disabled('all')
@benchmark.Owner(emails=['enne@chromium.org'])
class DrawPropertiesCT(_DrawProperties):

  @classmethod
  def Name(cls):
    return 'draw_properties_ct'

  @classmethod
  def AddBenchmarkCommandLineArgs(cls, parser):
    ct_benchmarks_util.AddBenchmarkCommandLineArgs(parser)

  @classmethod
  def ProcessCommandLineArgs(cls, parser, args):
    ct_benchmarks_util.ValidateCommandLineArgs(parser, args)

  def CreateStorySet(self, options):
    return contrib.cluster_telemetry.page_set.CTPageSet(
        options.urls_list, options.user_agent, options.archive_data_file)
