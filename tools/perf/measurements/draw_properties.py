# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry.page import legacy_page_test
from telemetry.timeline import model
from telemetry.timeline import tracing_config
from telemetry.value import scalar


class DrawProperties(legacy_page_test.LegacyPageTest):

  def __init__(self):
    super(DrawProperties, self).__init__()

  def CustomizeBrowserOptions(self, options):
    options.AppendExtraBrowserArgs([
        '--enable-prefer-compositing-to-lcd-text',
    ])

  def WillNavigateToPage(self, page, tab):
    del page  # unused
    config = tracing_config.TracingConfig()
    config.chrome_trace_config.category_filter.AddIncludedCategory(
        'cc')
    config.enable_chrome_trace = True
    tab.browser.platform.tracing_controller.StartTracing(config)

  def ComputeAverageOfDurations(self, timeline_model, name):
    events = timeline_model.GetAllEventsOfName(name)
    event_durations = [d.duration for d in events]

    duration_sum = sum(event_durations)
    duration_count = len(event_durations)
    if duration_count == 0 :
        return 0
    duration_avg = duration_sum / duration_count
    return duration_avg

  def GetCount(self, timeline_model, name):
    events = timeline_model.GetAllEventsOfName(name)
    event_durations = [d.duration for d in events]

    duration_count = len(event_durations)
    return duration_count

  def ValidateAndMeasurePage(self, page, tab, results):
    del page  # unused
    timeline_data = tab.browser.platform.tracing_controller.StopTracing()
    timeline_model = model.TimelineModel(timeline_data)

    bpt_avg = self.ComputeAverageOfDurations(
        timeline_model,
        'LayerTreeHostInProcess::UpdateLayers::BuildPropertyTrees')
    cdp_avg = self.ComputeAverageOfDurations(
        timeline_model,
        'LayerTreeImpl::UpdateDrawProperties::CalculateDrawProperties')
    root = len(timeline_model.GetAllEventsOfName('Root'))
    hide = len(timeline_model.GetAllEventsOfName('HideSubtree'))
    nest_hide = len(timeline_model.GetAllEventsOfName('NestedHideSubtree'))
    opacity = len(timeline_model.GetAllEventsOfName('Opacity'))
    pot_opacity = len(timeline_model.GetAllEventsOfName('PotentialOpacity'))
    proxied_opacity = len(timeline_model.GetAllEventsOfName('Proxied'))
    rs = len(timeline_model.GetAllEventsOfName('RS'))

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'PT_build_cost', 'ms', bpt_avg,
        description='Average time spent building property trees'))

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'Impl_cdp_cost', 'ms', cdp_avg,
        description='Average time spent calculating draw properties'))

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'num_root', '', root,
        description='Num root'))

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'num_hide_subtree', '', hide,
        description='Num Hidden Subtree'))

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'num_nested_hide_subtree', '', nest_hide,
        description='Num Nested Hidden Subtree'))

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'num_opacity', '', opacity,
        description='Num opacity'))

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'num_potential_op', '', pot_opacity,
        description='Num Potential opacity'))

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'num_proxied_op', '', proxied_opacity,
        description='Num Proxied opacity'))

    results.AddValue(scalar.ScalarValue(
        results.current_page, 'rs', '', rs,
        description='Num Render Surfaces'))

  def DidRunPage(self, platform):
    tracing_controller = platform.tracing_controller
    if tracing_controller.is_tracing_running:
      tracing_controller.StopTracing()
