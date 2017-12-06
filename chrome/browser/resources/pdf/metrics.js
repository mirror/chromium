// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

// Keep in sync with enums.xml.
// Do not change the numeric values or reuse them since these numbers are
// persisted to logs.
const UserAction = {
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

/**
 * Handles events specific to the PDF viewer and logs the corresponding metrics.
 *
 * @interface
 */
window.PDFMetrics = class PDFMetrics {
  /**
   * Creates a new PDFMetrics.
   * @constructor
   */
  constructor() {}

  /**
   * Call when the document is first loaded. This event serves as denominator to
   * determine percentages of documents in which an action was taken as well as
   * average number of each action per document.
   */
  onDocumentOpened() {}

  /**
   * Call when the document is rotated clockwise or counter-clockwise.
   */
  onRotation() {}

  /**
   * Call when the zoom mode is changed to fit a FittingType.
   * @param {FittingType} fittingType the new FittingType.
   */
  onFitTo(fittingType) {}

  /**
   * Call when the bookmarks panel is opened.
   */
  onOpenBookmarksPanel() {}

  /**
   * Call when a bookmark is followed.
   */
  onFollowBookmark() {}

  /**
   * Call when the page selection is used to navigate to another page.
   */
  onPageSelectorNavigation() {}
};

/**
 * Dummy implementation of PDFMetrics.
 */
window.PDFMetricsDummy = class PDFMetrics {
  constructor() {}

  onDocumentOpened() {}

  onRotation() {}

  onFitTo(fittingType) {}

  onOpenBookmarksPanel() {}

  onFollowBookmark() {}

  onPageSelectorNavigation() {}
};

/**
 * Implementation of PDFMetrics that logs the corresponding metrics to UMA
 * through chrome.metricsPrivate.
 */
window.PDFMetricsImpl = class PDFMetricsImpl extends PDFMetrics {
  /**
   * Creates a new PDFMetricsImpl.
   * @constructor
   */
  constructor() {
    super();

    /**
     * @type {Set}
     */
    this.firstEventLogged_ = new Set();

    /**
     * @type {Object}
     */
    this.actionsMetric_ = {
      'metricName': 'PDF.Actions',
      'type': chrome.metricsPrivate.MetricTypeType.HISTOGRAM_LOG,
      'min': 0,
      'max': PDFMetrics.UserAction.NUMBER_OF_ACTIONS - 1,
      'buckets': PDFMetrics.UserAction.NUMBER_OF_ACTIONS
    };
  }

  onDocumentOpened() {
    this.logOnlyFirstTime_(PDFMetrics.UserAction.DOCUMENT_OPENED);
  }

  onRotation() {
    this.logFirstAndTotal_(
        PDFMetrics.UserAction.ROTATE_FIRST, PDFMetrics.UserAction.ROTATE);
  }

  onFitTo(fittingType) {
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
  }

  onOpenBookmarksPanel() {
    this.logFirstAndTotal_(
        PDFMetrics.UserAction.OPEN_BOOKMARKS_PANEL_FIRST,
        PDFMetrics.UserAction.OPEN_BOOKMARKS_PANEL);
  }

  onFollowBookmark() {
    this.logFirstAndTotal_(
        PDFMetrics.UserAction.FOLLOW_BOOKMARK_FIRST,
        PDFMetrics.UserAction.FOLLOW_BOOKMARK);
  }

  onPageSelectorNavigation() {
    this.logFirstAndTotal_(
        PDFMetrics.UserAction.PAGE_SELECTOR_NAVIGATE_FIRST,
        PDFMetrics.UserAction.PAGE_SELECTOR_NAVIGATE);
  }

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
  logFirstAndTotal_(firstEventCode, totalEventCode) {
    this.log_(totalEventCode);
    this.logOnlyFirstTime_(firstEventCode);
  }

  /**
   * @private
   * Logs the given event code to chrome.metricsPrivate.
   * @param {number} eventCode event code to log.
   */
  log_(eventCode) {
    chrome.metricsPrivate.recordValue(this.actionsMetric_, eventCode);
  }

  /**
   * @private
   * Logs the given event code. Subsequent calls of this method with the same
   * event code have no effect on the this PDFMetrics instance.
   * @param {number} eventCode event code to log.
   */
  logOnlyFirstTime_(eventCode) {
    if (!this.firstEventLogged_.has(eventCode)) {
      this.log_(eventCode);
      this.firstEventLogged_.add(eventCode);
    }
  }
};

window.PDFMetrics.UserAction = UserAction;

}());
