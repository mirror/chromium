// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

/**
 * Implementation of PDFMetrics that logs the corresponding metrics to UMA
 * through chrome.metricsPrivate.
 */
window.PDFCoordsTransformer = class {
  constructor(postMessageCallback) {
    /**
     * @private {Array<Object>}
     */
    this.outstandingTransformPagePointRequests_ = [];
    this.postMessageCallback = postMessageCallback;
  }

  request(callback, params, page, x, y) {
    this.outstandingTransformPagePointRequests_.push(
        {callback: callback, params: params});
    this.postMessageCallback(
        {type: 'transformPagePoint', page: page, x: x, y: y});
  }

  onReplyReceived(message) {
    var outstandingRequest =
        this.outstandingTransformPagePointRequests_.shift();
    outstandingRequest.callback(message.data);
  }

  /**
   * @private
   * Logs the given event code to chrome.metricsPrivate.
   * @param {number} eventCode event code to log.
   */
  // log_(eventCode) {
  //   chrome.metricsPrivate.recordValue(this.actionsMetric_, eventCode);
  // }
};

}());
