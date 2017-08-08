# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from telemetry import value
from telemetry.web_perf.metrics import timeline_based_metric


class WebVRFpsMetric(timeline_based_metric.TimelineBasedMetric):
  """Reports FPS from trace event."""

  def AddResults(self, model, renderer_thread, interactions, results):
    browser = model.browser_process
    if not browser:
      return

    self._AddTraceEvents('VrShellGl::DrawFrame', 'draw frame', 'draw frame time', browser, results)
    self._AddTraceEvents('VrShellGl::AcquireFrame', 'acquire frame', 'acquire frame time', browser, results)
    self._AddTraceEvents('VrShellGl::UpdateController', 'update controller', 'update controller time', browser, results)
    self._AddTraceEvents('VrShellGl::DrawWorldElements', 'draw world elements', 'draw world elements time', browser, results)
    self._AddTraceEvents('VrShellGl::DrawFrameSubmitWhenReady', 'draw submit frame when ', 'acquire frame time', browser, results)
    self._AddTraceEvents('VrShellGl::DrawUiView', 'draw UI view', 'draw UI view time', browser, results)

  def _AddTraceEvents(self, trace_name, name, description, browser, results):
    events = [e for e in browser.parent.IterAllEvents(
       event_predicate=lambda event: event.name == trace_name)]
    values = [e.duration for e in events]
    
    if len(events) > 0:
      results.AddValue(value.list_of_scalar_values.ListOfScalarValues(
        page=results.current_page,
        name=name,
        units='ms',
        values=values,
        description=description,
        improvement_direction=value.improvement_direction.DOWN))

