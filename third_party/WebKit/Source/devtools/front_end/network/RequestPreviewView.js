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
   * @param {!UI.Widget} responseView
   */
  constructor(request, responseView) {
    super(request);
    this._responseView = responseView;
    /** @type {?Promise<!UI.Widget>} */
    this._previewView = null;
  }

  /**
   * @override
   */
  contentLoaded() {
    if (!this._previewView)
      this._previewView = this._createPreviewView();
    this._previewView.then(this._showPreviewView.bind(this));
  }

  /**
   * @param {!UI.Widget} previewView
   */
  _showPreviewView(previewView) {
    if (previewView.parentWidget())
      return;

    previewView.show(this.element);

    if (previewView instanceof UI.SimpleView) {
      var toolbar = new UI.Toolbar('network-item-preview-toolbar', this.element);
      for (var item of previewView.syncToolbarItems())
        toolbar.appendToolbarItem(item);
    }
  }

  /**
   * @return {!UI.EmptyWidget}
   */
  _createEmptyWidget() {
    return this._createMessageView(Common.UIString('This request has no preview available.'));
  }

  /**
   * @param {string} message
   * @return {!UI.EmptyWidget}
   */
  _createMessageView(message) {
    return new UI.EmptyWidget(message);
  }

  /**
   * @return {string}
   */
  _requestContent() {
    var content = this.request.content;
    return this.request.contentEncoded ? window.atob(content || '') : (content || '');
  }

  /**
   * @param {?Network.ParsedJSON} parsedJSON
   * @return {?UI.SearchableView}
   */
  _jsonView(parsedJSON) {
    if (!parsedJSON || typeof parsedJSON.data !== 'object')
      return null;
    return Network.JSONView.createSearchableView(/** @type {!Network.ParsedJSON} */ (parsedJSON));
  }

  /**
   * @return {?UI.SearchableView}
   */
  _xmlView() {
    var parsedXML = Network.XMLView.parseXML(this._requestContent(), this.request.mimeType);
    return parsedXML ? Network.XMLView.createSearchableView(parsedXML) : null;
  }

  /**
   * @return {?Network.RequestHTMLView}
   */
  _htmlErrorPreview() {
    var whitelist = ['text/html', 'text/plain', 'application/xhtml+xml'];
    if (whitelist.indexOf(this.request.mimeType) === -1)
      return null;

    var dataURL = this.request.asDataURL();
    if (dataURL === null)
      return null;

    return new Network.RequestHTMLView(this.request, dataURL);
  }

  /**
   * @return {!Promise<!UI.Widget>}
   */
  async _createPreviewView() {
    if (this.request.contentError())
      return this._createMessageView(Common.UIString('Failed to load response data'));

    var xmlView = this._xmlView();
    if (xmlView)
      return xmlView;

    // We support non-strict JSON parsing by parsing an AST tree which is why we offload it to a worker.
    var jsonData = await Network.JSONView.parseJSON(this._requestContent());

    if (jsonData) {
      var jsonView = this._jsonView(jsonData);
      if (jsonView)
        return jsonView;
    }

    if (this.request.hasErrorStatusCode() || this.request.resourceType() === Common.resourceTypes.XHR) {
      var htmlErrorPreview = this._htmlErrorPreview();
      if (htmlErrorPreview)
        return htmlErrorPreview;
    }

    if (this._responseView.sourceView)
      return this._responseView.sourceView;

    if (this.request.resourceType() === Common.resourceTypes.Other)
      return this._createEmptyWidget();

    return Network.RequestView.nonSourceViewForRequest(this.request);
  }
};
