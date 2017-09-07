// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var LineChartTest = LineChartTest || {};

LineChartTest.UnitLabel = function() {
  suite('LineChart Label unit test', function() {
    suiteSetup(function() {
      TEST_MEM_UNIT = ['B', 'KB', 'MB', 'GB', 'TB', 'PB'];
      TEST_MEM_UNITBASE = 1024;
      testLabel = new LineChart.UnitLabel(TEST_MEM_UNIT, TEST_MEM_UNITBASE);
    });

    test('getSuitableUnit', function() {
      const testValue1 = 1024 * 1024 * 1024 * 1024 * 5;
      const result1 = LineChart.UnitLabel.getSuitableUnit(
          testValue1, TEST_MEM_UNIT, TEST_MEM_UNITBASE);
      assertDeepEquals(result1, {value: 5, unitIdx: 4});

      const testValue2 = 1024 * 1024 * 1023;
      const result2 = LineChart.UnitLabel.getSuitableUnit(
          testValue2, TEST_MEM_UNIT, TEST_MEM_UNITBASE);
      assertDeepEquals(result2, {value: 1023, unitIdx: 2});

      const testValue3 = 1024 * 1024 * 1024 * 1024 * 1024 * 1024;
      const result3 = LineChart.UnitLabel.getSuitableUnit(
          testValue3, TEST_MEM_UNIT, TEST_MEM_UNITBASE);
      assertDeepEquals(result3, {value: 1024, unitIdx: 5});
    });

    test('getTopLabelValue_', function() {
      const result1 = testLabel.getTopLabelValue_(55, 10);
      assertEquals(result1, 60);
      const result2 = testLabel.getTopLabelValue_(73.5, 15);
      assertEquals(result2, 75);
    });

    test('UnitLabel integration test', function() {
      testLabel.setLayout(600, 12, 2);
      assertEquals(testLabel.getMaxNumberOfLabel_(), 6);
      assertEquals(testLabel.getCurrentUnitString(), 'B');
      assertEquals(testLabel.getRealValueWithCurrentUnit_(1234), 1234);

      const unit = 1024 * 1024 * 1024 * 1024;
      const maxValue = unit * 123;
      testLabel.setMaxValue(maxValue);
      assertEquals(testLabel.getCurrentUnitString(), 'TB');
      assertEquals(testLabel.getRealValueWithCurrentUnit_(42), unit * 42);

      assertEquals(testLabel.getNumberOfLabelWithStepSize_(20), 8);
      assertEquals(testLabel.getNumberOfLabelWithStepSize_(50), 4);
      assertEquals(testLabel.getNumberOfLabelWithStepSize_(0.1), 1231);

      assertDeepEquals(
          testLabel.getSuitableStepSize_(),
          {stepSize: 50, stepSizePrecision: 0});
      assertDeepEquals(
          testLabel.getLabels(), ['150 TB', '100 TB', '50 TB', '0 TB']);

      const realTopValue = 150 * unit;
      TestUtil.assertCloseTo(testLabel.getScale() * realTopValue, 600, 1e-2);
    });
  });

  mocha.run();
};
