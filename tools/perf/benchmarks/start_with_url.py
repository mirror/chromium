# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from measurements import startup
import page_sets
from telemetry import benchmark


@benchmark.Enabled('has tabs')
@benchmark.Disabled('chromeos', 'linux', 'mac', 'win')
class StartWithUrlCold(benchmark.Benchmark):
  """Measure time to start Chrome cold with startup URLs"""
  tag = 'cold'
  test = startup.StartWithUrl(cold=True)
  page_set = page_sets.StartupPagesPageSet
  options = {'pageset_repeat': 5}


@benchmark.Enabled('has tabs')
@benchmark.Disabled('chromeos', 'linux', 'mac', 'win')
class StartWithUrlWarm(benchmark.Benchmark):
  """Measure time to start Chrome warm with startup URLs"""
  tag = 'warm'
  test = startup.StartWithUrl(cold=False)
  page_set = page_sets.StartupPagesPageSet
  options = {'pageset_repeat': 10}
