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

    events = [e for e in browser.parent.IterAllEvents(
       event_predicate=lambda event: event.name == 'gpu.WebVR FPS')]
    fps = [e.value for e in events]
    if len(events) > 0:
      results.AddValue(value.list_of_scalar_values.ListOfScalarValues(
        page=results.current_page,
        name='webvr_fps',
        units='fps',
        values=fps,
        description=('frame rate'),
        improvement_direction=value.improvement_direction.UP))

