// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var LineChartTest = LineChartTest || {};

LineChartTest.LineChart = function() {
  test('touchDistance', function() {
    const touchA = {clientX: 100, clientY: 100};
    const touchB = {clientX: 140, clientY: 130};
    TestUtil.assertCloseTo(
        LineChart.LineChart.touchDistance_(touchA, touchB), 50, 1e-2);

    const touchC = {clientX: 1230, clientY: 475};
    const touchD = {clientX: 523, clientY: 675};
    TestUtil.assertCloseTo(
        LineChart.LineChart.touchDistance_(touchC, touchD), 734.744, 1e-2);
  });

  test('getSuitableTimeStep_', function() {
    assertEquals(LineChart.LineChart.getSuitableTimeStep_(100, 1000), 300000);
    assertEquals(LineChart.LineChart.getSuitableTimeStep_(70, 1500), 300000);
    assertEquals(LineChart.LineChart.getSuitableTimeStep_(120, 30000), 3600000);
    assertEquals(LineChart.LineChart.getSuitableTimeStep_(90, 100), 30000);
  });

  test('LineChart integration test', function() {
    const rootDiv = document.createElement('div');
    rootDiv.style = 'position: absolute; width: 1000px; height: 600px;';
    document.body.appendChild(rootDiv);

    const data1 = new LineChart.DataSeries('test1', '#aabbcc');
    data1.addDataPoint(100, 1504764694799);
    data1.addDataPoint(100, 1504764695799);
    data1.addDataPoint(100, 1504764696799);
    const data2 = new LineChart.DataSeries('test2', '#aabbcc');
    data2.addDataPoint(40, 1504764694799);
    data2.addDataPoint(42, 1504764695799);
    data2.addDataPoint(40, 1504764696799);
    const data3 = new LineChart.DataSeries('test3', '#aabbcc');
    data3.addDataPoint(1024, 1504764694799);
    data3.addDataPoint(2048, 1504764695799);
    data3.addDataPoint(4096, 1504764696799);

    const lineChart = new LineChart.LineChart(rootDiv);
    assertFalse(lineChart.shouldBeRender());
    lineChart.setSubChart(
        LineChart.UnitLabelAlign.LEFT, ['', 'K', 'M', 'G'], 1000);
    lineChart.setSubChart(
        LineChart.UnitLabelAlign.RIGHT, ['B', 'KB', 'MB', 'GB'], 1024);
    lineChart.addDataSeries(LineChart.UnitLabelAlign.LEFT, data1);
    lineChart.addDataSeries(LineChart.UnitLabelAlign.RIGHT, data2);
    lineChart.addDataSeries(LineChart.UnitLabelAlign.RIGHT, data3);
    assertTrue(lineChart.shouldBeRender());

    lineChart.updateEndTime(lineChart.startTime_ + 100000);

    lineChart.zoom(7.6);
    lineChart.zoom(1.42);
    lineChart.zoom(0.21);
    lineChart.zoom(1.25);
    lineChart.scroll(1044);
    lineChart.scroll(-50);
    lineChart.scroll(10.85);
    lineChart.scroll(-100);

    lineChart.clearAllSubChart();
    assertFalse(lineChart.shouldBeRender());
  });

  mocha.run();
};
