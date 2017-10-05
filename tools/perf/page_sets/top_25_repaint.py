# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
from telemetry.page import legacy_page_test
from telemetry.page import page as page_module
from telemetry.page import shared_page_state
from telemetry import story


from page_sets import top_pages


def _Repaint(action_runner, mode='viewport', width=None, height=None):
  action_runner.WaitForJavaScriptCondition(
    'document.readyState == "complete"', timeout=90)
  # Rasterize only what's visible.
  action_runner.ExecuteJavaScript(
      'chrome.gpuBenchmarking.setRasterizeOnlyVisibleContent();')

  args = {}
  args['mode'] = mode
  if width:
    args['width'] = width
  if height:
    args['height'] = height

  # Enqueue benchmark
  action_runner.ExecuteJavaScript("""
      window.benchmark_results = {};
      window.benchmark_results.id =
          chrome.gpuBenchmarking.runMicroBenchmark(
              "invalidation_benchmark",
              function(value) {},
              {{ args }}
          );
      """,
      args=args)

  micro_benchmark_id = action_runner.EvaluateJavaScript(
      'window.benchmark_results.id')
  if not micro_benchmark_id:
    raise legacy_page_test.MeasurementFailure(
        'Failed to schedule invalidation_benchmark.')

  with action_runner.CreateInteraction('Repaint'):
    action_runner.RepaintContinuously(seconds=2)

  action_runner.ExecuteJavaScript("""
      window.benchmark_results.message_handled =
          chrome.gpuBenchmarking.sendMessageToMicroBenchmark(
                {{ micro_benchmark_id }}, {
                  "notify_done": true
                });
      """,
      micro_benchmark_id=micro_benchmark_id)

class TopRepaintPage(page_module.Page):

  def __init__(self, url, page_set, base_dir=None, name='', credentials=None):
    if name == '':
      name = url
    super(TopRepaintPage, self).__init__(
        url=url, page_set=page_set, name=name,
        shared_page_state_class=shared_page_state.SharedDesktopPageState)

  def RunPageInteractions(self, action_runner):
    _Repaint(action_runner)

STATIC_TOP_25_DIR = 'static_top_25'

class Top25RepaintPageSet(story.StorySet):

  """ Page set consists of static top 25 pages. """

  def __init__(self):
    super(Top25RepaintPageSet, self).__init__(
        serving_dirs=[STATIC_TOP_25_DIR],
        cloud_storage_bucket=story.PARTNER_BUCKET)
    files = [
        'amazon.html',
        'blogger.html',
        'booking.html',
        'cnn.html',
        'ebay.html',
        'espn.html',
        'facebook.html',
        'gmail.html',
        'googlecalendar.html',
        'googledocs.html',
        'google.html',
        'googleimagesearch.html',
        'googleplus.html',
        'linkedin.html',
        'pinterest.html',
        'techcrunch.html',
        'twitter.html',
        'weather.html',
        'wikipedia.html',
        'wordpress.html',
        'yahooanswers.html',
        'yahoogames.html',
        'yahoonews.html',
        'yahoosports.html',
        'youtube.html'
        ]
    for f in files:
        url = "file://%s/%s" % (STATIC_TOP_25_DIR, f)
        self.AddStory(
            TopRepaintPage(url, self, self.base_dir))
