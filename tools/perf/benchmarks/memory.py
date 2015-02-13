# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from measurements import memory
import page_sets
from telemetry import benchmark


@benchmark.Enabled('android')
class MemoryMobile(benchmark.Benchmark):
  test = memory.Memory
  page_set = page_sets.MobileMemoryPageSet

  @classmethod
  def Name(cls):
    return 'memory.mobile_memory'


@benchmark.Disabled  # http://crbug.com/440305
class MemoryTop7Stress(benchmark.Benchmark):
  """Use (recorded) real world web sites and measure memory consumption."""
  test = memory.Memory
  page_set = page_sets.Top7StressPageSet

  @classmethod
  def Name(cls):
    return 'memory.top_7_stress'


@benchmark.Disabled
class Reload2012Q3(benchmark.Benchmark):
  """Memory consumption for a set of top pages from 2012.

  Performs reloading and garbage collecting on each page load."""
  tag = 'reload'
  test = memory.Memory
  page_set = page_sets.Top2012Q3StressPageSet

  @classmethod
  def Name(cls):
    return 'memory.reload.top_desktop_sites_2012Q3_stress'


@benchmark.Disabled  # crbug.com/371153
class MemoryToughDomMemoryCases(benchmark.Benchmark):
  test = memory.Memory
  page_set = page_sets.ToughDomMemoryCasesPageSet

  @classmethod
  def Name(cls):
    return 'memory.tough_dom_memory_cases'


@benchmark.Disabled('chromeos', 'linux', 'mac', 'win')
@benchmark.Enabled('has tabs')
class TypicalMobileSites(benchmark.Benchmark):
  """Long memory test."""
  test = memory.Memory
  page_set = page_sets.TypicalMobileSitesPageSet
  options = {'pageset_repeat': 15}
  @classmethod
  def Name(cls):
    return 'memory.typical_mobile_sites'

