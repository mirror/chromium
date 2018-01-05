// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

'use strict';

/**
 * Transforms page to screen coordinates using messages to the plugin.
 */
window.PDFCoordsTransformer = class {
  constructor(postMessageCallback) {
    /**
     * @private {Array<Object>}
     */
    this.outstandingTransformPagePointRequests_ = [];

    /**
     * @private {function(Object):void}
     */
    this.postMessageCallback_ = postMessageCallback;
  }

  /**
   * Send a 'transformPagePoint' message to the plugin.
   * @param {function(Object, Object):void} callback Function to call when the
   *     response is received.
   * @param {Object} params User parameters to be used in |callback|.
   * @param {number} page 0-based page number of the page where the point is.
   * @param {number} x x coordinate of the point.
   * @param {number} y y coordinate of the point.
   */
  request(callback, params, page, x, y) {
    this.outstandingTransformPagePointRequests_.push(
        {callback: callback, params: params});
    this.postMessageCallback_(
        {type: 'transformPagePoint', page: page, x: x, y: y});
  }

  /**
   * Call when 'transformPagePointReply' is received from the plugin.
   * @param {Object} message The message received from the plugin.
   */
  onReplyReceived(message) {
    var outstandingRequest =
        this.outstandingTransformPagePointRequests_.shift();
    outstandingRequest.callback(message.data, outstandingRequest.params);
  }
};

}());
