# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from metrics import timeline as timeline_module
from metrics import timeline_interaction_record as tir_module
from telemetry.page import page_measurement
from telemetry.core.timeline import model as model_module



# TimelineBasedMeasurement considers all instrumentation as producing a single
# timeline. But, depending on the amount of instrumentation that is enabled,
# overhead increases. The user of the measurement must therefore chose between
# a few levels of instrumentation.
NO_OVERHEAD_LEVEL = 'no-overhead'
MINIMAL_OVERHEAD_LEVEL = 'minimal-overhead'
DEBUG_OVERHEAD_LEVEL = 'debug-overhead'

ALL_OVERHEAD_LEVELS = [
  NO_OVERHEAD_LEVEL,
  MINIMAL_OVERHEAD_LEVEL,
  DEBUG_OVERHEAD_LEVEL
]


class _TimelineBasedMetrics(object):
  def __init__(self, model, renderer_thread):
    self._model = model
    self._renderer_thread = renderer_thread

  def FindTimelineInteractionRecords(self):
    # TODO(nduca): Add support for page-load interaction record.
    return [tir_module.TimelineInteractionRecord(event) for
            event in self._renderer_thread.IterAllAsyncSlices()
            if tir_module.IsTimelineInteractionRecord(event.name)]

  def CreateMetricsForTimelineInteractionRecord(self, interaction):
    res = []
    if interaction.is_smooth:
      pass # TODO(nduca): res.append smoothness metric instance.
    return res

  def AddResults(self, results):
    interactions = self.FindTimelineInteractionRecords()
    if len(interactions) == 0:
      raise Exception('Expected at least one Interaction on the page')
    for interaction in interactions:
      metrics = self.CreateMetricsForTimelineInteractionRecord(interaction)
      for m in metrics:
        m.AddResults(self._model, self._renderer_thread,
                     interaction, results)


class TimelineBasedMeasurement(page_measurement.PageMeasurement):
  """Collects multiple metrics pages based on their interaction records.

  A timeline measurement shifts the burden of what metrics to collect onto the
  page under test, or the pageset running that page. Instead of the measurement
  having a fixed set of values it collects about the page, the page being tested
  issues (via javascript) an Interaction record into the user timing API that
  describing what the page is doing at that time, as well as a standardized set
  of flags describing the semantics of the work being done. The
  TimelineBasedMeasurement object collects a trace that includes both these
  interaction recorsd, and a user-chosen amount of performance data using
  Telemetry's various timeline-producing APIs, tracing especially.

  It then passes the recorded timeline to different TimelineBasedMetrics based
  on those flags. This allows a single run through a page to produce load timing
  data, smoothness data, critical jank information and overall cpu usage
  information.

  For information on how to mark up a page to work with
  TimelineBasedMeasurement, refer to the
  perf.metrics.timeline_interaction_record module.

  """
  def __init__(self):
    super(TimelineBasedMeasurement, self).__init__('smoothness')

  def AddCommandLineOptions(self, parser):
    parser.add_option(
        '--overhead-level', type='choice',
        choices=ALL_OVERHEAD_LEVELS,
        default=NO_OVERHEAD_LEVEL,
        help='How much overhead to incur during the measurement.')

  def CanRunForPage(self, page):
    return hasattr(page, 'smoothness')

  def WillNavigateToPage(self, page, tab):
    if not tab.browser.supports_tracing:
      raise Exception('Not supported')
    assert self.options.overhead_level in ALL_OVERHEAD_LEVELS
    if self.options.overhead_level == NO_OVERHEAD_LEVEL:
      categories = timeline_module.MINIMAL_TRACE_CATEGORIES
    elif self.options.overhead_level == \
        MINIMAL_OVERHEAD_LEVEL:
      categories = ''
    else:
      categories = '*,disabled-by-default-cc.debug'
    tab.browser.StartTracing(categories)

  def MeasurePage(self, page, tab, results):
    """ Collect all possible metrics and added them to results. """
    trace_result = tab.browser.StopTracing()
    model = model_module.TimelineModel(trace_result)
    renderer_thread = model.GetRendererThreadFromTab(tab)
    meta_metrics = _TimelineBasedMetrics(model, renderer_thread)
    meta_metrics.AddResults(results)
