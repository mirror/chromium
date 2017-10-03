# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The tab switching measurement.

This measurement record the MPArch.RWH_TabSwitchPaintDuration histogram
of each tab swithcing.
"""

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
    self._first_vmstat = None

  def CustomizeBrowserOptions(self, options):
    """Adding necessary browser flag to collect histogram."""
    options.AppendExtraBrowserArgs(['--enable-stats-collection-bindings'])

  def DidNavigateToPage(self, page, tab):
    """Record the starting histogram."""
    self._first_histogram = cros_utils.GetTabSwitchHistogramRetry(tab.browser)
    self._first_vmstat = cros_utils.CrosVmstat(page.GetRemote())

  def ValidateAndMeasurePage(self, page, tab, results):
    """Record the ending histogram for the tab switching metric."""
    last_histogram = cros_utils.GetTabSwitchHistogramRetry(tab.browser)
    total_diff_histogram = histogram_util.SubtractHistogram(
        last_histogram, self._first_histogram)
    last_vmstat = cros_utils.CrosVmstat(page.GetRemote())

    display_name = 'MPArch_RWH_TabSwitchPaintDuration'
    results.AddSummaryValue(
        histogram.HistogramValue(
            None, display_name, 'ms',
            raw_value_json=total_diff_histogram,
            important=False))

    ret = cros_utils.CrosMeminfo(page.GetRemote())
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'Swap Total', 'kB', ret['SwapTotal']))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'Swap In', '',
        last_vmstat['pswpin'] - self._first_vmstat['pswpin']))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'Swap Out', '',
        last_vmstat['pswpout'] - self._first_vmstat['pswpout']))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'Major Fault', '',
        last_vmstat['pgmajfault'] - self._first_vmstat['pgmajfault']))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'Tabset Repeat', '',
        page.GetRepeatCount()))

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'Swap Used', 'kB',
        ret['SwapTotal'] - ret['SwapFree']))
    results.AddValue(scalar.ScalarValue(
        results.current_page, 'Anonymous', 'kB',
        ret['Active(anon)'] + ret['Inactive(anon)']))
