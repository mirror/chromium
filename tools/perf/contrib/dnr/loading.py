# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from benchmarks import loading

from telemetry import benchmark
import page_sets
from telemetry.page import cache_temperature

@benchmark.Owner(emails=['karandeepb@chromium.org'])
class LoadingDesktopDNR(loading.LoadingDesktop):
  """Measures loading performance of desktop sites, with the network service
  enabled.
  """
  @classmethod
  def Name(cls):
    return 'loading.desktop.dnr'

  def CreateStorySet(self, options):
      return page_sets.MyPageSet(
          cache_temperatures=[cache_temperature.COLD, cache_temperature.WARM])

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs(
          '--load-extension=/Users/karandeepb/Desktop/chromium/src/chrome/test/data/extensions/declarative_net_request/dwr_unpacked')
    options.AppendExtraBrowserArgs(
          '--enable-automation')

@benchmark.Owner(emails=['karandeepb@chromium.org'])
class LoadingDesktopUblock(loading.LoadingDesktop):
  """Measures loading performance of desktop sites, with the network service
  enabled.
  """
  @classmethod
  def Name(cls):
    return 'loading.desktop.ublock'

  def CreateStorySet(self, options):
      return page_sets.MyPageSet(
          cache_temperatures=[cache_temperature.COLD, cache_temperature.WARM])

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs(
          '--load-extension=/Users/karandeepb/Desktop/chromium/src/chrome/test/data/extensions/declarative_net_request/ublock_dev')
    options.AppendExtraBrowserArgs(
          '--enable-automation')

@benchmark.Owner(emails=['karandeepb@chromium.org'])
class LoadingDesktopNone(loading.LoadingDesktop):
  """Measures loading performance of desktop sites, with the network service
  enabled.
  """
  @classmethod
  def Name(cls):
    return 'loading.desktop.none'

  def CreateStorySet(self, options):
      return page_sets.MyPageSet(
          cache_temperatures=[cache_temperature.COLD, cache_temperature.WARM])

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs(
          '--load-extension=/Users/karandeepb/Desktop/chromium/src/chrome/test/data/extensions/declarative_net_request/ublock_dev')
    options.AppendExtraBrowserArgs(
          '--enable-automation')
