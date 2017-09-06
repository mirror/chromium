// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var LineChartTest = LineChartTest || {};

LineChartTest.Label = function() {
  suite('LineChart Label unit test', function() {
    suiteSetup(function() {
      TEST_MEM_UNIT = ['B', 'KB', 'MB', 'GB', 'TB', 'PB'];
      TEST_MEM_UNITBASE = 1024;
      testLabel = new LineChart.Label(TEST_MEM_UNIT, TEST_MEM_UNITBASE);
    });

    test('getSuitableUnit', function() {
      const testValue1 = 1024 * 1024 * 1024 * 1024 * 5;
      const result1 = LineChart.Label.getSuitableUnit(
          testValue1, TEST_MEM_UNIT, TEST_MEM_UNITBASE);
      assertDeepEquals(result1, {value: 5, unitIdx: 4});

      const testValue2 = 1024 * 1024 * 1023;
      const result2 = LineChart.Label.getSuitableUnit(
          testValue2, TEST_MEM_UNIT, TEST_MEM_UNITBASE);
      assertDeepEquals(result2, {value: 1023, unitIdx: 2});

      const testValue3 = 1024 * 1024 * 1024 * 1024 * 1024 * 1024;
      const result3 = LineChart.Label.getSuitableUnit(
          testValue3, TEST_MEM_UNIT, TEST_MEM_UNITBASE);
      assertDeepEquals(result3, {value: 1024, unitIdx: 5});
    });

    test
  });

  mocha.run();
};
