# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The tab switching measurement.

This measurement record the MPArch.RWH_TabSwitchPaintDuration histogram
of each tab swithcing.
"""
import time

from telemetry.page import legacy_page_test
from telemetry.value import histogram
from telemetry.value import histogram_util
from telemetry.value import scalar

from contrib.cros_benchmarks import cros_utils


class CrosTabSwitchingMeasurement(legacy_page_test.LegacyPageTest):
  """Measures tab switching performance."""

  def __init__(self):
    super(CrosTabSwitchingMeasurement, self).__init__()
    self._first_histogram = None
    self._first_reload = None
    self._first_vmstat = None
    self._switch_start = None

  def CustomizeBrowserOptions(self, options):
    """Adding necessary browser flag to collect histogram."""
    options.AppendExtraBrowserArgs(['--enable-stats-collection-bindings'])

  def DidNavigateToPage(self, page, tab):
    """Record the starting histogram."""
    self._first_histogram = cros_utils.GetTabSwitchHistogramRetry(tab.browser)
    self._first_vmstat = cros_utils.CrosVmstat(page.GetRemote())
    self._switch_start = time.time()
    self._first_reload = cros_utils.GetReloadCount(tab.browser)

  def ValidateAndMeasurePage(self, page, tab, results):
    """Record the ending histogram for the tab switching metric."""
    last_histogram = cros_utils.GetTabSwitchHistogramRetry(tab.browser)
    total_diff_histogram = histogram_util.SubtractHistogram(
        last_histogram, self._first_histogram)
    switch_time = time.time() - self._switch_start

    display_name = 'MPArch_RWH_TabSwitchPaintDuration'
    results.AddSummaryValue(
        histogram.HistogramValue(
            None, display_name, 'ms',
            raw_value_json=total_diff_histogram,
            important=False))
    results.AddSummaryValue(scalar.ScalarValue(
        None, 'Switch Total Time', 'sec', switch_time))
    results.AddSummaryValue(scalar.ScalarValue(
        None, 'Tabset Repeat', 'count', page.GetRepeatCount()))

    last_reload = cros_utils.GetReloadCount(tab.browser)
    results.AddSummaryValue(scalar.ScalarValue(
        None, 'Reload Count', 'count', last_reload - self._first_reload))

    self.LogStatus(results, page.GetRemote())

  def LogStatus(self, results, dut_ip):
    last_vmstat = cros_utils.CrosVmstat(dut_ip)
    mem_info = cros_utils.CrosMeminfo(dut_ip)
    zram_info = cros_utils.CrosZramInfo(dut_ip)

    swap_used = mem_info['SwapTotal'] - mem_info['SwapFree']
    anon_pages = mem_info['Active(anon)'] + mem_info['Inactive(anon)']
    file_pages = mem_info['Active(file)'] + mem_info['Inactive(file)']

    def GetVMDiff(name):
      return last_vmstat[name] - self._first_vmstat[name]
    def GetVMDiffPrefix(prefix):
      result = 0
      for key in last_vmstat.keys():
        if key.startswith(prefix):
          result += GetVMDiff(key)
      return result

    swap_in = GetVMDiff('pswpin')
    swap_out = GetVMDiff('pswpout')
    maj_fault = GetVMDiff('pgmajfault')
    pgscan_kswapd = GetVMDiffPrefix('pgscan_kswapd_')
    pgscan_direct = GetVMDiffPrefix('pgscan_direct_')
    pgsteal_kswapd = GetVMDiffPrefix('pgsteal_kswapd_')
    pgsteal_direct = GetVMDiffPrefix('pgsteal_direct_')
    pgalloc = GetVMDiffPrefix('pgalloc_')
    pgfree = GetVMDiff('pgfree')
    pgrefill = GetVMDiffPrefix('pgrefill_')
    wmark_hit_quickly = GetVMDiff('kswapd_low_wmark_hit_quickly')

    def AddScalarValue(name, unit, value):
      results.AddSummaryValue(scalar.ScalarValue(
          None, name, unit, value))

    AddScalarValue('Memory Total', 'KB', mem_info['MemTotal'])
    AddScalarValue('Swap Total', 'KB', mem_info['SwapTotal'])
    AddScalarValue('Swap Used', 'KB', swap_used)
    AddScalarValue('Anonymous Pages', 'KB', anon_pages)
    AddScalarValue('File Pages', 'KB', file_pages)

    AddScalarValue('Swap In', 'count', swap_in)
    AddScalarValue('Swap Out', 'count', swap_out)
    AddScalarValue('Major Fault', 'count', maj_fault)
    AddScalarValue('Page Scan Kswapd', 'count', pgscan_kswapd)
    AddScalarValue('Page Scan Direct', 'count', pgscan_direct)
    AddScalarValue('Page Steal Kswapd', 'count', pgsteal_kswapd)
    AddScalarValue('Page Steal Direct', 'count', pgsteal_direct)
    AddScalarValue('Page Alloc', 'count', pgalloc)
    AddScalarValue('Page Free', 'count', pgfree)
    AddScalarValue('Page Refill', 'count', pgrefill)
    AddScalarValue('Watermark Hit Quickly', 'count', wmark_hit_quickly)

    AddScalarValue('Zram Memory Used', 'bytes', zram_info['mem_used_total'])
    AddScalarValue('Zram Compressed', 'bytes', zram_info['compr_data_size'])
    AddScalarValue('Zram Original', 'bytes', zram_info['orig_data_size'])
