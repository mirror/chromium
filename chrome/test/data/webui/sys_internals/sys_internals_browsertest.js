// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Test suite for the SysInternals WebUI. (CrOS only)
 */
const ROOT_PATH = '../../../../../';

function SysInternalsBrowserTest() {}

SysInternalsBrowserTest.prototype = {
  __proto__: testing.Test.prototype,

  browsePreload: 'chrome://sys-internals',

  runAccessibilityChecks: false,

  isAsync: true,

  commandLineSwitches:
      [{switchName: 'enable-features', switchValue: 'SysInternals'}],

  extraLibraries: [
    'api_test.js',
    'info_page_test.js',
    'line_chart/data_series_test.js',
    'line_chart/line_chart_test.js',
    'line_chart/unit_label_test.js',
    'test_util.js',
    ROOT_PATH + 'third_party/mocha/mocha.js',
    ROOT_PATH + 'chrome/test/data/webui/mocha_adapter.js',
  ],
};

TEST_F('SysInternalsBrowserTest', 'getSysInfo', function() {
  ApiTest.getSysInfo();
});

TEST_F('SysInternalsBrowserTest', 'LineChart_DataSeries', function() {
  LineChartTest.DataSeries();
});

TEST_F('SysInternalsBrowserTest', 'LineChart_LineChart', function() {
  LineChartTest.LineChart();
});

TEST_F('SysInternalsBrowserTest', 'LineChart_UnitLabel', function() {
  LineChartTest.UnitLabel();
});

TEST_F('SysInternalsBrowserTest', 'Page_InfoPage', function() {
  PageTest.InfoPage();
});
