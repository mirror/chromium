# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from benchmarks import loading

from telemetry import benchmark


@benchmark.Owner(emails=['karandeepb@chromium.org'])
class LoadingDesktopDNR(loading.LoadingDesktop):
  """Measures loading performance of desktop sites, with the network service
  enabled.
  """
  @classmethod
  def Name(cls):
    return 'loading.desktop.dnr'

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs(
          '--load-extension=/Users/karandeepb/Desktop/chromium/src/chrome/test/data/extensions/declarative_net_request/dwr_unpacked')


@benchmark.Owner(emails=['karandeepb@chromium.org'])
class LoadingDesktopUblock(loading.LoadingDesktop):
  """Measures loading performance of desktop sites, with the network service
  enabled.
  """
  @classmethod
  def Name(cls):
    return 'loading.desktop.ublock'

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs(
          '--load-extension=/Users/karandeepb/Desktop/chromium/src/chrome/test/data/extensions/declarative_net_request/ublock_dev')

