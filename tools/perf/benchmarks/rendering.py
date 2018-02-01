# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from core import perf_benchmark

import page_sets
from telemetry import benchmark
from telemetry import story as story_module


@benchmark.Owner(emails=['vmiura@chromium.org'])
class RenderingDesktop(perf_benchmark.PerfBenchmark):

  page_set = page_sets.RenderingDesktopPageSet
  SUPPORTED_PLATFORMS = [story_module.expectations.ALL_DESKTOP]

  @classmethod
  def Name(cls):
    return 'rendering.desktop'


@benchmark.Owner(emails=['vmiura@chromium.org'])
class RenderingMobile(perf_benchmark.PerfBenchmark):

  page_set = page_sets.RenderingMobilePageSet
  SUPPORTED_PLATFORMS = [story_module.expectations.ALL_MOBILE]

  @classmethod
  def Name(cls):
    return 'rendering.mobile'
