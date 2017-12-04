// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

chrome.test.runTests(function() {
  'use strict';

  var DOCUMENT_OPENED = 0;  // Baseline to use as denominator for all formulas.
  var ROTATE_FIRST_ACTION = 1;
  var ROTATE_ACTION = 2;
  var FIT_TO_WIDTH_FIRST_ACTION = 3;
  var FIT_TO_WIDTH_ACTION = 4;
  var FIT_TO_PAGE_FIRST_ACTION = 5;
  var FIT_TO_PAGE_ACTION = 6;
  var BOOKMARKS_OPENED_FIRST_ACTION = 7;
  var BOOKMARKS_OPENED_ACTION = 8;
  var BOOKMARK_FOLLOWED_FIRST_ACTION = 9;
  var BOOKMARK_FOLLOWED_ACTION = 10;
  var PAGE_SELECTOR_NAVIGATE_FIRST_ACTION = 11;
  var PAGE_SELECTOR_NAVIGATE_ACTION = 12;
  var NUMBER_OF_ACTIONS = 13;

  class MockMetricsPrivate {
    constructor() {
      this.MetricTypeType = {HISTOGRAM_LOG: 'test_histogram_log'};
      this.actionCounter = {};
    }

    recordValue(metric, value) {
      chrome.test.assertEq('PDF.Actions', metric.metricName);
      chrome.test.assertEq('test_histogram_log', metric.type);
      chrome.test.assertEq(0, metric.min);
      chrome.test.assertEq(NUMBER_OF_ACTIONS - 1, metric.max);
      chrome.test.assertEq(NUMBER_OF_ACTIONS, metric.buckets);
      this.actionCounter[value] = (this.actionCounter[value] + 1) || 1;
    }
  };

  return [
    function testMetricsOnDocumentOpened() {
      chrome.metricsPrivate = new MockMetricsPrivate();
      let metrics = new PDFMetrics();
      metrics.onDocumentOpened();

      chrome.test.assertEq(
          {[DOCUMENT_OPENED]:1},
          chrome.metricsPrivate.actionCounter);
      chrome.test.succeed();
    },

    function testMetricsOnRotation() {
      chrome.metricsPrivate = new MockMetricsPrivate();
      let metrics = new PDFMetrics();
      metrics.onDocumentOpened();
      for (var i = 0; i < 4; i++)
        metrics.onRotation();

      chrome.test.assertEq(
          {
              [DOCUMENT_OPENED]: 1,
              [ROTATE_FIRST_ACTION]: 1,
              [ROTATE_ACTION]: 4
          },
          chrome.metricsPrivate.actionCounter);
      chrome.test.succeed();
    },

    function testMetricsOnFitTo() {
      chrome.metricsPrivate = new MockMetricsPrivate();
      let metrics = new PDFMetrics();
      metrics.onDocumentOpened();
      metrics.onFitTo(FittingType.FIT_TO_HEIGHT);
      metrics.onFitTo(FittingType.FIT_TO_PAGE);
      metrics.onFitTo(FittingType.FIT_TO_WIDTH);
      metrics.onFitTo(FittingType.FIT_TO_PAGE);
      metrics.onFitTo(FittingType.FIT_TO_WIDTH);
      metrics.onFitTo(FittingType.FIT_TO_PAGE);

      chrome.test.assertEq(
          {
              [DOCUMENT_OPENED]: 1,
              [FIT_TO_PAGE_FIRST_ACTION]: 1,
              [FIT_TO_PAGE_ACTION]: 3,
              [FIT_TO_WIDTH_FIRST_ACTION]: 1,
              [FIT_TO_WIDTH_ACTION]: 2
          },
          chrome.metricsPrivate.actionCounter);
      chrome.test.succeed();
    },

    function testMetricsOnBookmarksOpenedAndFollowed() {
      chrome.metricsPrivate = new MockMetricsPrivate();
      let metrics = new PDFMetrics();
      metrics.onDocumentOpened();

      metrics.onBookmarksOpened();
      metrics.onBookmarkFollowed();
      metrics.onBookmarkFollowed();

      metrics.onBookmarksOpened();
      metrics.onBookmarkFollowed();
      metrics.onBookmarkFollowed();
      metrics.onBookmarkFollowed();

      chrome.test.assertEq(
          {
              [DOCUMENT_OPENED]: 1,
              [BOOKMARKS_OPENED_FIRST_ACTION]: 1,
              [BOOKMARKS_OPENED_ACTION]: 2,
              [BOOKMARK_FOLLOWED_FIRST_ACTION]: 1,
              [BOOKMARK_FOLLOWED_ACTION]: 5
          },
          chrome.metricsPrivate.actionCounter);
      chrome.test.succeed();
    },

    function testMetricsOnPageSelectorNavigation() {
      chrome.metricsPrivate = new MockMetricsPrivate();
      let metrics = new PDFMetrics();
      metrics.onDocumentOpened();

      metrics.onPageSelectorNavigation();
      metrics.onPageSelectorNavigation();

      chrome.test.assertEq(
          {
              [DOCUMENT_OPENED]: 1,
              [PAGE_SELECTOR_NAVIGATE_FIRST_ACTION]: 1,
              [PAGE_SELECTOR_NAVIGATE_ACTION]: 2
          },
          chrome.metricsPrivate.actionCounter);
      chrome.test.succeed();
    },
  ];
}());
