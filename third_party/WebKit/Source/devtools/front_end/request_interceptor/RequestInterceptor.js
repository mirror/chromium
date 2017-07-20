// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

RequestInterceptor.RequestInterceptor = class {
  constructor() {
    /** @type {!Map<!RequestInterceptor.InterceptReceiver, !RegExp>} */
    this._interceptors = new Map();

    SDK.multitargetNetworkManager.addEventListener(
        SDK.MultitargetNetworkManager.Events.RequestIntercepted, this._onRequestIntercepted, this);
  }

  /**
   * @param {!Common.Event} event
   */
  async _onRequestIntercepted(event) {
    var requestInterception = /** @type {!SDK.NetworkManager.RequestInterception} */ (event.data);
    var interceptionResponse = new RequestInterceptor.SharedInterceptionResponse(requestInterception);
    var interceptors = Array.from(this._interceptors.entries());
    for (var entry of interceptors) {
      if (!entry[1].test(interceptionResponse.url))
        continue;
      await entry[0](interceptionResponse);
    }
    requestInterception.manager.continueInterceptedRequest(interceptionResponse);
  }

  /**
   * @param {string} pattern
   * @param {!RequestInterceptor.InterceptReceiver} callback
   */
  registerInterceptor(pattern, callback) {
    this._interceptors.set(callback, this._patternToRegEx(pattern));
  }

  /**
   * @param {!RequestInterceptor.InterceptReceiver} callback
   */
  unregisterInterceptor(callback) {
    this._interceptors.delete(callback);
  }

  /**
   * @param {boolean} enabled
   */
  setEnabled(enabled) {
    SDK.multitargetNetworkManager.setRequestInterceptionEnabled(enabled);
  }

  /**
   * @param {string} pattern
   * @return {!RegExp}
   */
  _patternToRegEx(pattern) {
    return new RegExp('^' + pattern.split('*').map(part => part.escapeForRegExp()).join('.*') + '$', 'i');
  }
};

/** @typedef {function(!RequestInterceptor.SharedInterceptionResponse):!Promise} */
RequestInterceptor.InterceptReceiver;

RequestInterceptor.requestInterceptor = new RequestInterceptor.RequestInterceptor();

RequestInterceptor.SharedInterceptionResponse = class {
  /**
   * @param {!SDK.NetworkManager.RequestInterception} requestInterception
   */
  constructor(requestInterception) {
    this.interceptionId = requestInterception.interceptionId;
    this.url = requestInterception.redirectUrl || requestInterception.request.url;
    this.method = requestInterception.request.method;
    this.postData = requestInterception.request.postData;
    this.headers = requestInterception.request.headers;
    /** @type {string|undefined} */
    this.originUrl;

    /** @type {!Protocol.Network.ErrorReason|undefined} */
    this.errorReason;
    /** @type {string|undefined} */
    this.rawResponse;
    /** @type {!Protocol.Network.AuthChallengeResponse|undefined} */
    this.authChallengeResponse;
  }
};
