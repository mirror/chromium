// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var PDFMetrics;

(function() {

'use strict';

/**
 * Creates a new PDFMetrics. This logs metrics for user actions to UMA.
 * @constructor
 */
PDFMetrics = function() {
  if (!chrome.metricsPrivate)
    return;

  this.firstEventLogged = new Set();
  this.actionsMetric = {
    'metricName': 'PDF.Actions',
    'type': chrome.metricsPrivate.MetricTypeType.HISTOGRAM_LOG,
    'min': 0,
    'max': PDFMetrics.UserAction.NUMBER_OF_ACTIONS - 1,
    'buckets': PDFMetrics.UserAction.NUMBER_OF_ACTIONS
  };
};

// Keep in sync with enums.xml.
PDFMetrics.UserAction = {
  DOCUMENT_OPENED: 0,  // Baseline to use as denominator for all formulas.
  ROTATE_FIRST: 1,
  ROTATE: 2,
  FIT_TO_WIDTH_FIRST: 3,
  FIT_TO_WIDTH: 4,
  FIT_TO_PAGE_FIRST: 5,
  FIT_TO_PAGE: 6,
  OPEN_BOOKMARKS_PANEL_FIRST: 7,
  OPEN_BOOKMARKS_PANEL: 8,
  FOLLOW_BOOKMARK_FIRST: 9,
  FOLLOW_BOOKMARK: 10,
  PAGE_SELECTOR_NAVIGATE_FIRST: 11,
  PAGE_SELECTOR_NAVIGATE: 12,
  NUMBER_OF_ACTIONS: 13
};

PDFMetrics.prototype = {

  /**
   * Call when the document is first loaded. This even serves as denominator to
   * determine percentages of documents in which an action was taken as well as
   * average number of each action per document.
   */
  onDocumentOpened: function() {
    if (!chrome.metricsPrivate)
      return;

    chrome.metricsPrivate.recordValue(
        this.actionsMetric, PDFMetrics.UserAction.DOCUMENT_OPENED);
  },

  /**
   * Call when the document is rotated clockwise or counter-clockwise.
   */
  onRotation: function() {
    this.logFirstAndTotal_(
        PDFMetrics.UserAction.ROTATE_FIRST, PDFMetrics.UserAction.ROTATE);
  },

  /**
   * Call when the zoom mode is changed to fit a FittingType.
   * @param {FittingType} fittingType the new FittingType.
   */
  onFitTo: function(fittingType) {
    if (fittingType == FittingType.FIT_TO_PAGE) {
      this.logFirstAndTotal_(
          PDFMetrics.UserAction.FIT_TO_PAGE_FIRST,
          PDFMetrics.UserAction.FIT_TO_PAGE);
    } else if (fittingType == FittingType.FIT_TO_WIDTH) {
      this.logFirstAndTotal_(
          PDFMetrics.UserAction.FIT_TO_WIDTH_FIRST,
          PDFMetrics.UserAction.FIT_TO_WIDTH);
    }
    // There is no user action to do a fit-to-height, this only happens with
    // the open param "view=FitV".
  },

  /**
   * Call when the bookmarks panel is opened.
   */
  onOpenBookmarksPanel: function() {
    this.logFirstAndTotal_(
        PDFMetrics.UserAction.OPEN_BOOKMARKS_PANEL_FIRST,
        PDFMetrics.UserAction.OPEN_BOOKMARKS_PANEL);
  },

  /**
   * Call when a bookmark is followed.
   */
  onFollowBookmark: function() {
    this.logFirstAndTotal_(
        PDFMetrics.UserAction.FOLLOW_BOOKMARK_FIRST,
        PDFMetrics.UserAction.FOLLOW_BOOKMARK);
  },

  /**
   * Call when the page selection is used to navigate to another page.
   */
  onPageSelectorNavigation: function() {
    this.logFirstAndTotal_(
        PDFMetrics.UserAction.PAGE_SELECTOR_NAVIGATE_FIRST,
        PDFMetrics.UserAction.PAGE_SELECTOR_NAVIGATE);
  },

  /**
   * @private
   * Logs the "first" event code if it hasn't been logged by this instance yet
   * and also log the "total" event code. This distinction allows analyzing
   * both:
   * - in what percentage of documents each action was taken;
   * - how many times, on average, each action is taken on a document;
   * @param {number} firstEventCode event code for the "first" metric.
   * @return {number} totalEventCode event code for the "total"  metric.
   */
  logFirstAndTotal_: function(firstEventCode, totalEventCode) {
    if (!chrome.metricsPrivate)
      return;

    chrome.metricsPrivate.recordValue(this.actionsMetric, totalEventCode);

    if (!this.firstEventLogged.has(firstEventCode)) {
      chrome.metricsPrivate.recordValue(this.actionsMetric, firstEventCode);
      this.firstEventLogged.add(firstEventCode);
    }
  },
};

}());
