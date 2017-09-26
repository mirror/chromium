// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

SourceFrame.PreviewFactory = class {
  /**
   * @param {!Common.ContentProvider} provider
   * @param {string} mimeType
   * @returns {!Promise<?UI.Widget>}
   */
  static async createPreview(provider, mimeType) {
    var content = await provider.requestContent();
    return this.createPreviewWithContentData(provider, {error: null, content: content, encoded: false}, mimeType);
  }

  /**
   * @param {!Common.ContentProvider} originalProvider
   * @param {!SDK.NetworkRequest.ContentData} contentData
   * @param {string} mimeType
   * @returns {!Promise<?UI.Widget>}
   */
  static async createPreviewWithContentData(originalProvider, contentData, mimeType) {
    if (contentData.content === null || contentData.error)
      return new UI.EmptyWidget(Common.UIString('Failed to load resource data'));

    try {
      var content = (contentData.encoded ? atob(contentData.content || '') : contentData.content) || '';
    } catch (e) {
      console.error('Could not decode content: ' + e);
      return new UI.EmptyWidget(Common.UIString('Failed to decode response'));
    }
    mimeType = mimeType.replace(/;.*/, '');  // remove charset.

    if (!content)
      return new UI.EmptyWidget(Common.UIString('Nothing to preview'));

    // We support non-strict JSON parsing by parsing an AST tree which is why we offload it to a worker.
    var parsedJSON = await SourceFrame.JSONView.parseJSON(content);
    if (parsedJSON && typeof parsedJSON.data === 'object')
      return SourceFrame.JSONView.createSearchableView(/** @type {!SourceFrame.ParsedJSON} */ (parsedJSON));

    // We can assume the status code has been set already because fetching contentData should wait for request to be
    // finished.
    var whitelist = new Set(['text/html', 'text/plain', 'application/xhtml+xml']);
    if (whitelist.has(mimeType)) {
      var dataURL = Common.ContentProvider.contentAsDataURL(
          content, mimeType, contentData.encoded, contentData.encoded ? 'utf-8' : null);
      return dataURL ? new SourceFrame.HTMLView(dataURL) : null;
    }

    var parsedXML = SourceFrame.XMLView.parseXML(content, mimeType);
    if (parsedXML)
      return SourceFrame.XMLView.createSearchableView(parsedXML);

    var resourceType = originalProvider.contentType() || Common.resourceTypes.Other;

    if (resourceType.isTextType())
      return SourceFrame.ResourceSourceFrame.createSearchableView(originalProvider, mimeType);

    switch (resourceType) {
      case Common.resourceTypes.Image:
        return new SourceFrame.ImageView(mimeType, originalProvider);
      case Common.resourceTypes.Font:
        return new SourceFrame.FontView(mimeType, originalProvider);
    }
    return null;
  }
};
