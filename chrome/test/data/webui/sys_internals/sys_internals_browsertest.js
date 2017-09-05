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
    'line_chart/line_chart_test.js',
    ROOT_PATH + 'third_party/mocha/mocha.js',
    ROOT_PATH + 'chrome/test/data/webui/mocha_adapter.js',
  ],

  /** @override */
  setUp: function() {
    testing.Test.prototype.setUp.call(this);
  },
};

TEST_F('SysInternalsBrowserTest', 'getSysInfo', function() {
  ApiTest.getSysInfo();
});

TEST_F('SysInternalsBrowserTest', 'LineChart_LineChart', function() {
  LineChartTest.LineChart()
});
