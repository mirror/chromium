# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from core import perf_benchmark
from benchmarks import blink_perf
from telemetry import story
from telemetry import page as page_module
from telemetry.page import legacy_page_test
from telemetry.page import shared_page_state
from telemetry.timeline import tracing_config
from telemetry.timeline.model import TimelineModel
from telemetry.value import list_of_scalar_values
from telemetry.value import scalar

class _BlinkBindingsPerfBase(legacy_page_test.LegacyPageTest):

  def __init__(self, action_name=''):
    super(_BlinkBindingsPerfBase, self).__init__(action_name)

  def WillNavigateToPage(self, page, tab):
    del page  # unused
    # FIXME: Remove webkit.console when blink.console lands in chromium and
    # the ref builds are updated. crbug.com/386847
    config = tracing_config.TracingConfig()
    for c in ['blink.console', 'v8']:
      config.chrome_trace_config.category_filter.AddIncludedCategory(c)
    config.enable_chrome_trace = True
    tab.browser.platform.tracing_controller.StartTracing(config, timeout=10000)

  def ValidateAndMeasurePage(self, page, tab, results):
    tab.WaitForJavaScriptCondition('testRunner.isDone', timeout=10000)
    del page  # unused
    timeline_data = tab.browser.platform.tracing_controller.StopTracing()
    timeline_model = TimelineModel(timeline_data)
    threads = timeline_model.GetAllThreads()
    for thread in threads:
      if thread.name == 'CrRendererMain':
        self.AddTracingResults(thread, results)

  def DidRunPage(self, platform):
    if platform.tracing_controller.is_tracing_running:
      platform.tracing_controller.StopTracing()

  def AddTracingResults(self, thread, results):
    pass


class BlinkBindingsWindowInitialize(_BlinkBindingsPerfBase):
  MAPPING = {
      'LocalWindowProxy::Initialize': 'init',
      'LocalWindowProxy::UpdateDocumentProperty': 'document',
      'LocalWindowProxy::CreateContext': 'context',
      'LocalWindowProxy::SetupWindowPrototypeChain': 'window',
      'ContextCreation': 'context::new',
      'ContextCreatedNotification': 'notification',
  }

  def __init__(self):
    super(BlinkBindingsWindowInitialize, self).__init__()
    with open(os.path.join(os.path.dirname(__file__),
                           'blink_perf.js'), 'r') as f:
      self._blink_perf_js = f.read()

  def WillNavigateToPage(self, page, tab):
    page.script_to_evaluate_on_commit = self._blink_perf_js
    super(BlinkBindingsWindowInitialize, self).WillNavigateToPage(page, tab)

  def ValidateAndMeasurePage(self, page, tab, results):
    super(BlinkBindingsWindowInitialize, self).ValidateAndMeasurePage(
        page, tab, results)

  @classmethod
  def CustomizeBrowserOptions(cls, options):
    # TODO(qinmin): After fixing crbug.com/592017, remove this command line.
    options.AppendExtraBrowserArgs([
        '--reduce-security-for-testing',
        '--enable-blink-features="V8ContextSnapshot"',
    ])

  def AddTracingResults(self, thread, results):
    non_main_frames = {}
    main_frame = {}

    for name in self.MAPPING.values():
      non_main_frames[name] = []

    for e in thread.all_slices:
      if e.name in self.MAPPING:
        key = self.MAPPING[e.name]
        if e.args['IsMainFrame']:
          main_frame[key] = e.duration
        else:
          non_main_frames[key].append(e.duration)
    page = results.current_page
    unit = 'ms'

    for name in self.MAPPING.values():
      if len(non_main_frames[name]) > 0:
        results.AddValue(list_of_scalar_values.ListOfScalarValues(
            page, 'non_main_' + name, unit, non_main_frames[name]))
      if name in main_frame:
        results.AddValue(scalar.ScalarValue(page, 'main_' + name,
                                            unit, main_frame[name]))


class BlinkBindingsBenchmarkWindowInitialize(perf_benchmark.PerfBenchmark):
  tag = 'window_initialize'
  test = BlinkBindingsWindowInitialize

  @classmethod
  def Name(cls):
    return 'blink_bindings.window_initialize'

  def CreateStorySet(self, options):
    path = os.path.join(blink_perf.BLINK_PERF_BASE_DIR,
                        'Bindings', 'window-initialize.html')
    url = 'file://' + path.replace('\\', '/')
    serving_dirs = set()
    if '../' in open(path, 'r').read():
      # If the page looks like it references its parent dir, include it.
      serving_dirs.add(os.path.dirname(os.path.dirname(path)))

    ps = story.StorySet(base_dir=os.getcwd() + os.sep,
                        serving_dirs=serving_dirs)
    ps.AddStory(page_module.Page(url, ps, ps.base_dir, name='InitializeWindowProxy',
        shared_page_state_class=(shared_page_state.SharedPageState)))
    return ps
