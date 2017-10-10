# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import multiprocessing

from core import perf_benchmark

from measurements import smoothness
import page_sets
import page_sets.asm_js_pages
from telemetry import benchmark


class AsmJs(perf_benchmark.PerfBenchmark):
  """asm.js benchmark."""

  @classmethod
  def Name(cls):
    return 'asmjs'

  def CreateStorySet(self, options):
    return page_sets.asm_js_pages.AsmJsPageSet()
