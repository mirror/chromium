# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json

from telemetry.value import histogram_util
from telemetry.value import list_of_scalar_values
from telemetry.value import scalar

from metrics import Metric

class UseCounterMetric(Metric):
  """UseCounterMetric trackers whether specific blink UseCounters were hit
  """

  HISTOGRAM_NAME = 'Blink.UseCounter.Features'

  PAGE_VISITS_BUCKET = 52

  LIMITED_BUCKETS_MODE = False

  # If the above is true, then use only the features specified here.
  # Telemetry doesn't appear to have any way of passing options into
  # a benchmark from the command line.
  BUCKET_VALUES = [
    0,     # PageDestruction
    52,    # PageVisits
    343,   # EventSrcElement
    677,   # XMLHttpRequestAsynchronous (a popular JS metric as a control)
    1075,  # V8SloppyMode
    1076,  # V8StrictMode
    1077,  # V8StrongMode
#    967,   # AddEventListenerThirdArgumentIsObject
#    968,   # RemoveEventListenerThirdArgumentIsObject
#    1279,  # AddEventListenerFourArguments
#    1280,  # RemoveEventListenerFourArguments
    575,  # WindowPostMessageWithLegacyTargetOriginArgument
  ]

  @classmethod
  def CustomizeBrowserOptions(cls, options):
    options.AppendExtraBrowserArgs(['--enable-stats-collection-bindings'])

  def __init__(self):
    super(UseCounterMetric, self).__init__()
    self._histogram_start = ""

  def Start(self, page, tab):
    self._histogram_start = histogram_util.GetHistogram(
        histogram_util.RENDERER_HISTOGRAM,
        UseCounterMetric.HISTOGRAM_NAME,
        tab)

  def AddResults(self, tab, results):
    """Adds the number of times the specified metrics were hit
    Should be 0 or 1 if UseCounter and the user story are functioning correctly.
    """

    histogram = histogram_util.GetHistogram(
        histogram_util.RENDERER_HISTOGRAM,
        UseCounterMetric.HISTOGRAM_NAME,
        tab)

    # Determine if the renderer has been restarted as a result of the
    # navigation. Normally checking the 'pid' should be sufficient, but the fix
    # for crbug.com/666912 may not be in the build under test, and besides it
    # may be possible (if unlikely) for a unique process ID to get recycled.
    # So also check for a decrease in the histogram.
    start_parsed = json.loads(self._histogram_start)
    end_parsed = json.loads(histogram)
    if start_parsed:
        start_page_visits = sum(b['count'] for b in start_parsed['buckets']
            if b['low'] == UseCounterMetric.PAGE_VISITS_BUCKET)
    else:
        start_page_visits = 0
    end_page_visits = sum(b['count'] for b in end_parsed['buckets']
        if b['low'] == UseCounterMetric.PAGE_VISITS_BUCKET)

    if (not start_parsed or
          start_parsed['pid'] != end_parsed['pid'] or
          start_parsed['sum'] > end_parsed['sum'] or
          start_page_visits > end_page_visits):
        # Just use the raw ending histogram
        histogram_delta = histogram
    else:
        # Use the difference between the start and end histograms
        histogram_delta = histogram_util.SubtractHistogram(histogram,
            self._histogram_start)

    buckets = histogram_util.GetHistogramBucketsFromJson(histogram_delta)

    if UseCounterMetric.LIMITED_BUCKETS_MODE:
        for bucketValue in UseCounterMetric.BUCKET_VALUES:
            useCount = sum(bucket['count'] for bucket in buckets
                                             if bucket['low'] == bucketValue)
            results.AddValue(scalar.ScalarValue(results.current_page,
                'Feature' + str(bucketValue), 'count', useCount))

    else:
        results.AddValue(list_of_scalar_values.ListOfScalarValues(
            results.current_page, 'features', 'list-of-ids',
            [b['low'] for b in buckets]))

