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

Network.RequestPreviewView = class extends Network.RequestContentView {
  /**
   * @param {!SDK.NetworkRequest} request
   */
  constructor(request) {
    super(request);
    /** @type {?Promise<!UI.Widget>} */
    this._previewView = null;
  }

  /**
   * @return {!UI.EmptyWidget}
   */
  static _createEmptyWidget() {
    return Network.RequestPreviewView._createMessageView(Common.UIString('This request has no preview available.'));
  }

  /**
   * @param {string} message
   * @return {!UI.EmptyWidget}
   */
  static _createMessageView(message) {
    return new UI.EmptyWidget(message);
  }

  /**
   * @param {string} content
   * @param {string} mimeType
   * @return {?UI.SearchableView}
   */
  static _xmlView(content, mimeType) {
    var parsedXML = Network.XMLView.parseXML(content, mimeType);
    return parsedXML ? Network.XMLView.createSearchableView(parsedXML) : null;
  }

  /**
   * @param {?Network.ParsedJSON} parsedJSON
   * @return {?UI.SearchableView}
   */
  static _jsonView(parsedJSON) {
    if (!parsedJSON || typeof parsedJSON.data !== 'object')
      return null;
    return Network.JSONView.createSearchableView(/** @type {!Network.ParsedJSON} */ (parsedJSON));
  }

  /**
   * @override
   */
  wasShown() {
    this._showPreviewView();
  }

  async _showPreviewView() {
    if (!this._previewView)
      this._previewView = this._createPreviewView();
    var previewView = await this._previewView;
    if (this.element.contains(previewView.element))
      return;

    previewView.show(this.element);

    if (previewView instanceof UI.SimpleView) {
      var toolbar = new UI.Toolbar('network-item-preview-toolbar', this.element);
      for (var item of previewView.syncToolbarItems())
        toolbar.appendToolbarItem(item);
    }

    this._previewDidShowForTest(previewView);
  }

  _previewDidShowForTest(view) {
  }

  /**
   * @return {!Promise<?Network.RequestHTMLView>}
   */
  async _htmlErrorPreview() {
    var whitelist = ['text/html', 'text/plain', 'application/xhtml+xml'];
    if (whitelist.indexOf(this.request.mimeType) === -1)
      return null;

    var responseData = await this.request.responseData();
    var dataURL = Common.ContentProvider.contentAsDataURL(
        responseData.content, this.request.mimeType, responseData.encoded, responseData.encoded ? 'utf-8' : null);
    if (dataURL === null)
      return null;

    return new Network.RequestHTMLView(this.request, /** @type {string} */ (dataURL));
  }

  /**
   * @return {!Promise<!UI.Widget>}
   */
  async _createPreviewView() {
    var responseData = await this.request.responseData();
    if (responseData.error)
      return Network.RequestPreviewView._createMessageView(Common.UIString('Failed to load response data'));

    var content = responseData.content || '';
    if (responseData.encoded)
      content = window.atob(content);
    if (!content)
      return Network.RequestPreviewView._createEmptyWidget();

    var xmlView = Network.RequestPreviewView._xmlView(content, this.request.mimeType);
    if (xmlView)
      return xmlView;

    // We support non-strict JSON parsing by parsing an AST tree which is why we offload it to a worker.
    var jsonData = await Network.JSONView.parseJSON(content);

    if (jsonData) {
      var jsonView = Network.RequestPreviewView._jsonView(jsonData);
      if (jsonView)
        return jsonView;
    }

    if (this.request.hasErrorStatusCode() || this.request.resourceType() === Common.resourceTypes.XHR) {
      var htmlErrorPreview = await this._htmlErrorPreview();
      if (htmlErrorPreview)
        return htmlErrorPreview;
    }

    var sourceView = await Network.RequestResponseView.sourceViewForRequest(this.request);
    if (sourceView)
      return sourceView;

    if (this.request.resourceType() === Common.resourceTypes.Other)
      return Network.RequestPreviewView._createEmptyWidget();

    return Network.RequestView.nonSourceViewForRequest(this.request);
  }
};
