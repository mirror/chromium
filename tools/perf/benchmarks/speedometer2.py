# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Apple's Speedometer 2 performance benchmark.
"""

import os

from core import path_util
from core import perf_benchmark

from telemetry import benchmark
from telemetry import page as page_module
from telemetry.page import legacy_page_test
from telemetry import story
from telemetry.value import list_of_scalar_values


_SPEEDOMETER_DIR = os.path.join(path_util.GetChromiumSrcDir(),
    'third_party', 'WebKit', 'PerformanceTests', 'Speedometer')


class Speedometer2Measurement(legacy_page_test.LegacyPageTest):
  def __init__(self):
    super(Speedometer2Measurement, self).__init__()

  def ValidateAndMeasurePage(self, page, tab, results):
    tab.WaitForDocumentReadyStateToBeComplete()
    iterationCount = 2
    # A single iteration on android takes ~75 seconds, the benchmark times out
    # when running for 10 iterations.
    if tab.browser.platform.GetOSName() == 'android':
      iterationCount = 3
    tab.ExecuteJavaScript("""
        // Store all the results in the benchmarkClient
        var testDone = false;
        var iterationCount = {{ count }};
        var benchmarkClient = {};
        var suiteValues = [];
        benchmarkClient.didRunSuites = function(measuredValues) {
          suiteValues.push(measuredValues);
        };
        benchmarkClient.didFinishLastIteration = function () {
          testDone = true;
        };
        var runner = new BenchmarkRunner(Suites, benchmarkClient);
        runner.runMultipleIterations(iterationCount);
        """,
        count=iterationCount)
    tab.WaitForJavaScriptCondition('testDone', timeout=600)


    # Extract the timings for all suites.
    # |suite_metrics| will be a dictionary whose keys are concatenate path
    # name of a metric (e.g: "Vanilla-ES2015-TodoMVC-CompletingAllItems-sync"),
    # and value is a list of collected results from all iterations.
    suite_metrics = tab.EvaluateJavaScript("""
        var suiteMetrics = {};

        function pushValueToSuiteMetrics(metricName, item) {
          if (metricName in suiteMetrics) {
            suiteMetrics[metricName].push(item);
          } else {
            suiteMetrics[metricName] = [item];
          }
        }

        // Recursively traverse the tree of test results & extract the metric
        // results to |suiteMetrics|.
        function extractSuiteMetricResults(suiteResults, baseName) {
          // Leaf node: suiteResults contains only action names & metric
          // values
          if (typeof suiteResults === 'number') {
            pushValueToSuiteMetrics(baseName, suiteResults);
            return;
          }
          // Non-leaf node: suiteResults contains "total" metric &
          // 'tests' dictionary which contain the children nodes.
          if (suiteResults.total)
             pushValueToSuiteMetrics(baseName + '-Total', suiteResults.total);
          for (var subResult in suiteResults['tests']) {
            extractSuiteMetricResults(
                suiteResults['tests'][subResult],
                baseName + '-' + subResult);
          }
        };

        for(var i = 0; i < iterationCount; i++) {
          pushValueToSuiteMetrics('Total', suiteValues[i].total);
          for (var subResult in suiteValues[i]['tests'])
            extractSuiteMetricResults(
                suiteValues[i]['tests'][subResult], subResult);
        };
        suiteMetrics;
        """)

    for metric_name, values in suite_metrics.iteritems():
      results.AddSummaryValue(list_of_scalar_values.ListOfScalarValues(
          None, metric_name, 'ms', values))

@benchmark.Owner(emails=['verwaest@chromium.org, mvstanton@chromium.org'])
class Speedometer2(perf_benchmark.PerfBenchmark):
  test = Speedometer2Measurement

  @classmethod
  def Name(cls):
    return 'speedometer2'

  def CreateStorySet(self, options):
    ps = story.StorySet(base_dir=_SPEEDOMETER_DIR,
        serving_dirs=[_SPEEDOMETER_DIR])
    ps.AddStory(page_module.Page(
       'file://InteractiveRunner.html', ps, ps.base_dir, name='Speedometer2'))
    return ps
