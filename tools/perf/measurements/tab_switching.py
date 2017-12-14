# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The tab switching measurement.

This measurement opens pages in different tabs. After all the tabs have opened,
it cycles through each tab in sequence, and records a histogram of the time
between when a tab was first requested to be shown, and when it was painted.
Power usage is also measured.
"""

from telemetry.page import legacy_page_test
from telemetry.value import histogram
from telemetry.value import histogram_util
from telemetry.value import improvement_direction
from telemetry.value import list_of_scalar_values
from telemetry.timeline import model as model_module
from telemetry.timeline import tracing_config

from metrics import keychain_metric


class TabSwitching(legacy_page_test.LegacyPageTest):
  def __init__(self):
    super(TabSwitching, self).__init__()
    self._first_histogram = None

  def CustomizeBrowserOptions(self, options):
    keychain_metric.KeychainMetric.CustomizeBrowserOptions(options)
    options.AppendExtraBrowserArgs(['--enable-stats-collection-bindings'])

  def WillNavigateToPage(self, page, tab):
    config = tracing_config.TracingConfig()
    config.enable_chrome_trace = True
    config.enable_platform_display_trace = True
    config.chrome_trace_config.category_filter.AddFilterString('latency')
    tab.browser.platform.tracing_controller.StartTracing(config)

  @classmethod
  def _GetTabSwitchHistogram(cls, tab_to_switch):
    histogram_name = 'MPArch.RWH_TabSwitchPaintDuration'
    histogram_type = histogram_util.BROWSER_HISTOGRAM
    return histogram_util.GetHistogram(
        histogram_type, histogram_name, tab_to_switch)

  def DidNavigateToPage(self, page, tab):
    """record the starting histogram"""
    self._first_histogram = self._GetTabSwitchHistogram(tab)

  def ValidateAndMeasurePage(self, page, tab, results):
    """record the ending histogram for the tab switching metric."""
    last_histogram = self._GetTabSwitchHistogram(tab)
    total_diff_histogram = histogram_util.SubtractHistogram(last_histogram,
                            self._first_histogram)
    display_name = 'MPArch_RWH_TabSwitchPaintDuration'
    results.AddSummaryValue(
        histogram.HistogramValue(None, display_name, 'ms',
            raw_value_json=total_diff_histogram,
            important=False))

    trace_result = tab.browser.platform.tracing_controller.StopTracing()
    if isinstance(trace_result, tuple):
      trace_result = trace_result[0]
    latencies = None
    model = model_module.TimelineModel(trace_result)
    if model.browser_process:
      process = model.browser_process
      latencies = [event.args['ms'] for event in
          process.IterAllEventsOfName('TabSwitching::Latency')]
    results.AddValue(list_of_scalar_values.ListOfScalarValues(
        page, 'tab_switch_to_paint', 'ms', latencies,
        description='Delay between requesting a tab to become to visible to '
                    'when the tab is made visible on screen.',
        none_value_reason=('Not enough data.'),
        improvement_direction=improvement_direction.DOWN))

    keychain_metric.KeychainMetric().AddResults(tab, results)
