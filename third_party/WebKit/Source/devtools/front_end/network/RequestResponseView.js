/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

Network.RequestResponseView = class extends Network.RequestContentView {
  /**
   * @param {!SDK.NetworkRequest} request
   */
  constructor(request) {
    super(request);
    /** @type {?Promise<!UI.Widget>} */
    this._responseView = null;
  }

  /**
   * @param {!SDK.NetworkRequest} request
   * @return {!Promise<?UI.SearchableView>}
   */
  static async sourceViewForRequest(request) {
    var sourceView = request[Network.RequestResponseView._sourceViewSymbol];
    if (sourceView !== undefined)
      return sourceView;


    var responseData = await request.responseData();
    if (!Network.RequestView.hasTextContent(request, responseData)) {
      request[Network.RequestResponseView._sourceViewSymbol] = null;
      return null;
    }

    var contentProvider = new Network.RequestResponseView.ContentProvider(request);
    var highlighterType = request.resourceType().canonicalMimeType() || request.mimeType;
    sourceView = SourceFrame.ResourceSourceFrame.createSearchableView(contentProvider, highlighterType);
    request[Network.RequestResponseView._sourceViewSymbol] = sourceView;
    return sourceView;
  }

  /**
   * @override
   */
  wasShown() {
    this._showResponseView();
  }

  async _showResponseView() {
    if (!this._responseView)
      this._responseView = this._createResponseView();
    var responseView = await this._responseView;
    if (this.element.contains(responseView.element))
      return;
    responseView.show(this.element);
  }

  /**
   * @return {!Promise<!UI.Widget>}
   */
  async _createResponseView() {
    var responseData = await this.request.responseData();
    if (responseData.error)
      return new UI.EmptyWidget(Common.UIString('Failed to load response data.'));

    if (!responseData.content)
      return new UI.EmptyWidget(Common.UIString('This request has no response data available.'));

    var sourceView = await Network.RequestResponseView.sourceViewForRequest(this.request);
    if (sourceView)
      return sourceView;

    return new UI.EmptyWidget(Common.UIString('Failed to load response data'));
  }
};

Network.RequestResponseView._sourceViewSymbol = Symbol('RequestResponseSourceView');

/**
 * @implements {Common.ContentProvider}
 */
Network.RequestResponseView.ContentProvider = class {
  /**
   * @param {!SDK.NetworkRequest} request
   */
  constructor(request) {
    this._request = request;
  }

  /**
   * @override
   * @return {string}
   */
  contentURL() {
    return this._request.contentURL();
  }

  /**
   * @override
   * @return {!Common.ResourceType}
   */
  contentType() {
    return this._request.resourceType();
  }

  /**
   * @override
   * @return {!Promise<?string>}
   */
  async requestContent() {
    var responseData = await this._request.responseData();
    return responseData.encoded ? window.atob(responseData.content || '') : responseData.content;
  }

  /**
   * @override
   * @param {string} query
   * @param {boolean} caseSensitive
   * @param {boolean} isRegex
   * @param {function(!Array.<!Common.ContentProvider.SearchMatch>)} callback
   */
  searchInContent(query, caseSensitive, isRegex, callback) {
    this._request.searchInContent(query, caseSensitive, isRegex, callback);
  }
};
