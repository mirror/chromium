// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var PDFMetrics;

(function() {

'use strict';

PDFMetrics = function() {
  this.firstEventLogged = new Set();
};

var DOCUMENT_OPENED = 0; // Baseline to use as denominator for all formulas.
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

var PDF_ACTIONS_METRIC = {
  'metricName': 'PDF.Actions',
  'type': chrome.metricsPrivate.MetricTypeType.HISTOGRAM_LOG,
  'min': 0,
  'max': NUMBER_OF_ACTIONS - 1,
  'buckets': NUMBER_OF_ACTIONS
};

PDFMetrics.prototype = {

  onDocumentOpened: function() {
    chrome.metricsPrivate.recordValue(PDF_ACTIONS_METRIC, DOCUMENT_OPENED);
  },

  onRotation: function() {
    this.logFirstAndTotal_(ROTATE_FIRST_ACTION, ROTATE_ACTION);
  },

  onFitTo: function(fittingType) {
    if (fittingType == FittingType.FIT_TO_PAGE)
      this.logFirstAndTotal_(FIT_TO_PAGE_FIRST_ACTION, FIT_TO_PAGE_ACTION);
    else if (fittingType == FittingType.FIT_TO_WIDTH)
      this.logFirstAndTotal_(FIT_TO_WIDTH_FIRST_ACTION, FIT_TO_WIDTH_ACTION);
    // There is no user action to do a fit-to-height, this only happens with
    // the open param "view=FitV".
  },

  onBookmarksOpened: function() {
    this.logFirstAndTotal_(
        BOOKMARKS_OPENED_FIRST_ACTION, BOOKMARKS_OPENED_ACTION);
  },

  onBookmarkFollowed: function() {
    this.logFirstAndTotal_(
        BOOKMARK_FOLLOWED_FIRST_ACTION, BOOKMARK_FOLLOWED_ACTION);
  },

  onPageSelectorNavigation: function() {
    this.logFirstAndTotal_(
        PAGE_SELECTOR_NAVIGATE_FIRST_ACTION, PAGE_SELECTOR_NAVIGATE_ACTION);
  },

  logFirstAndTotal_ : function(firstEventCode, totalEventCode) {
    chrome.metricsPrivate.recordValue(PDF_ACTIONS_METRIC, totalEventCode);

    if (!this.firstEventLogged.has(firstEventCode)) {
      chrome.metricsPrivate.recordValue(PDF_ACTIONS_METRIC, firstEventCode);
      this.firstEventLogged.add(firstEventCode);
    }
  },

};

}());
