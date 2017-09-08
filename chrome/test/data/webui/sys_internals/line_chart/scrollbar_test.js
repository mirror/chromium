// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var LineChartTest = LineChartTest || {};

LineChartTest.Scrollbar = function() {
  test('Scrollbar integration test', function() {
    const scrollbar = new LineChart.Scrollbar(function() {});
    scrollbar.resize(100);
    scrollbar.setRange(1000);
    assertFalse(scrollbar.isScrolledToRightEdge());
    scrollbar.scrollToRightEdge();
    assertTrue(scrollbar.isScrolledToRightEdge());
    scrollbar.setPosition(500);
    assertFalse(scrollbar.isScrolledToRightEdge());
    scrollbar.setRange(100);
    assertTrue(scrollbar.isScrolledToRightEdge());
  });

  mocha.run();
};
