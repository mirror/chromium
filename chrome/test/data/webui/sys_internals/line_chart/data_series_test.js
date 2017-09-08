// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var LineChartTest = LineChartTest || {};

LineChartTest.DataSeries = function() {
  suite('LineChart DataSeries unit test', function() {
    test('linerInterpolation', function() {
      const result1 =
          LineChart.DataSeries.linerInterpolation(100, 15, 142, 32, 118);
      TestUtil.assertCloseTo(result1, 22.285, 1e-2);
      const result2 =
          LineChart.DataSeries.linerInterpolation(42, 15, 142, 32, 65);
      TestUtil.assertCloseTo(result2, 18.91, 1e-2);
      const result3 = LineChart.DataSeries.linerInterpolation(
          100000, 640000, 123456, 654545, 112345);
      TestUtil.assertCloseTo(result3, 647655.099, 1e-2);
    });

    test('dataPointLinerInterpolation', function() {
      const dataSeries = new LineChart.DataSeries('Test', '#aabbcc');
      dataSeries.getValues(0, 100, 10);
      const pointA = {time: 100, value: 15};
      const pointB = {time: 142, value: 32};
      const result1 =
          dataSeries.dataPointLinerInterpolation(pointA, pointB, 118);
      TestUtil.assertCloseTo(result1, 23.5, 1e-2);

      dataSeries.getValues(30, 100, 10);
      const pointC = {time: 42, value: 15};
      const pointD = {time: 142, value: 32};
      const result21 =
          dataSeries.dataPointLinerInterpolation(pointC, pointD, 65);
      TestUtil.assertCloseTo(result21, 15, 1e-2);
      const result22 =
          dataSeries.dataPointLinerInterpolation(pointC, pointD, 135);
      TestUtil.assertCloseTo(result22, 32, 1e-2);

      dataSeries.getValues(42, 100, 10);
      const pointE = {time: 42, value: 615};
      const pointF = {time: 542, value: 132};
      const result3 =
          dataSeries.dataPointLinerInterpolation(pointE, pointF, 315);
      TestUtil.assertCloseTo(result3, 421.7999, 1e-2);
    });

    test('DataSeries integration test', function() {
      const dataSeries = new LineChart.DataSeries('Test', '#aabbcc');
      dataSeries.getValues(1200, 150, 10);  // Call when 0 point.
      dataSeries.addDataPoint(10, 1000);
      dataSeries.getValues(1200, 150, 10);  // Call when 1 point.
      dataSeries.addDataPoint(20, 2000);
      dataSeries.addDataPoint(42, 3000);
      dataSeries.addDataPoint(31, 4000);
      dataSeries.addDataPoint(59, 5000);
      dataSeries.addDataPoint(787, 6000);
      dataSeries.addDataPoint(612, 7000);
      dataSeries.addDataPoint(4873, 8000);
      dataSeries.addDataPoint(22, 9000);
      dataSeries.addDataPoint(10, 10000);

      assertEquals(dataSeries.findLowerBoundPointIndex_(1500), 1);
      assertEquals(dataSeries.findLowerBoundPointIndex_(9000), 8);
      assertEquals(dataSeries.findLowerBoundPointIndex_(10001), 10);

      assertDeepEquals(
          dataSeries.getSampleValue_(1, 2500), {value: 20, nextIndex: 2});
      assertDeepEquals(
          dataSeries.getSampleValue_(3, 3000), {value: null, nextIndex: 3});
      assertDeepEquals(
          dataSeries.getSampleValue_(4, 7000), {value: 423, nextIndex: 6});
      assertDeepEquals(
          dataSeries.getSampleValue_(10, 11000), {value: null, nextIndex: 10});

      const val1 = dataSeries.getValues(2000, 2000, 1);
      assertDeepEquals(val1, [31]);
      const max1 = dataSeries.getMaxValue(2000, 2000, 1);
      assertEquals(max1, 31);

      const val2 = dataSeries.getValues(0, 1000, 3);
      assertDeepEquals(val2, [null, 10, 20]);
      const max2 = dataSeries.getMaxValue(0, 1000, 3);
      assertEquals(max2, 20);

      const val3 = dataSeries.getValues(0, 3000, 1);
      /* (10 + 20) / 2 */
      assertDeepEquals(val3, [15]);
      const max3 = dataSeries.getMaxValue(0, 3000, 1);
      assertEquals(max3, 15);

      const val4 = dataSeries.getValues(4545, 1500, 5);
      assertDeepEquals(val4, [423, 612, 2447.5, 10, null]);
      const max4 = dataSeries.getMaxValue(4545, 1500, 5);
      assertEquals(max4, 2447.5);

      const val5 = dataSeries.getValues(1200, 100, 10);
      console.log(val5);
      assertDeepEquals(
          val5, [12, null, null, null, null, null, null, null, 20, 22.2]);
      const max5 = dataSeries.getMaxValue(1200, 100, 10);
      assertEquals(max5, 22.2);
    });
  });

  mocha.run();
};
