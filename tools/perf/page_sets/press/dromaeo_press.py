# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import math

from benchmarks import press
from telemetry.value import scalar


class DromaeoPress(press.PressPage):
  ABSTRACT_PAGE = True

  def StartTest(self, page, tab, results):
    tab.WaitForJavaScriptCondition(
        'window.document.getElementById("pause") &&' +
        'window.document.getElementById("pause").value == "Run"',
        timeout=120)

    # Start spying on POST request that will report benchmark results, and
    # intercept result data.
    tab.ExecuteJavaScript("""
        (function() {
          var real_jquery_ajax_ = window.jQuery;
          window.results_ = "";
          window.jQuery.ajax = function(request) {
            if (request.url == "store.php") {
              window.results_ = decodeURIComponent(request.data);
              window.results_ = window.results_.substring(
                window.results_.indexOf("=") + 1,
                window.results_.lastIndexOf("&"));
              real_jquery_ajax_(request);
            }
          };
        })();""")
    # Starts benchmark.
    tab.ExecuteJavaScript('window.document.getElementById("pause").click();')

  def WaitForTestToFinish(self, page, tab, results):
    tab.WaitForJavaScriptCondition('!!window.results_', timeout=600)

  def ParseTestResults(self, page, tab, results):
    score = json.loads(tab.EvaluateJavaScript('window.results_ || "[]"'))

    def Escape(k):
      chars = [' ', '.', '-', '/', '(', ')', '*']
      for c in chars:
        k = k.replace(c, '_')
      return k

    def AggregateData(container, key, value):
      if key not in container:
        container[key] = {'count': 0, 'sum': 0}
      container[key]['count'] += 1
      container[key]['sum'] += math.log(value)

    suffix = page.url[page.url.index('?') + 1:]

    def AddResult(name, value):
      important = False
      if name == suffix:
        important = True
      results.AddValue(scalar.ScalarValue(
          results.current_page, Escape(name), 'runs/s', value, important))

    aggregated = {}
    for data in score:
      AddResult('%s/%s' % (data['collection'], data['name']),
                data['mean'])

      top_name = data['collection'].split('-', 1)[0]
      AggregateData(aggregated, top_name, data['mean'])

      collection_name = data['collection']
      AggregateData(aggregated, collection_name, data['mean'])

    for key, value in aggregated.iteritems():
      AddResult(key, math.exp(value['sum'] / value['count']))


class DomCoreAttr(DromaeoPress):
  NAME = 'dromaeo.domcoreattr'
  URL = 'http://dromaeo.com?dom-attr'


class DomCoreModify(DromaeoPress):
  NAME = 'dromaeo.domcoremodify'
  URL = 'http://dromaeo.com?dom-modify'


class DomCoreQuery(DromaeoPress):
  NAME = 'dromaeo.domcorequery'
  URL = 'http://dromaeo.com?dom-query'


class DomCoreTraverse(DromaeoPress):
  NAME = 'dromaeo.domcoretraverse'
  URL = 'http://dromaeo.com?dom-traverse'
