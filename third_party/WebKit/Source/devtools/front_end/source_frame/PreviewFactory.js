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
    if (!content)
      return new UI.EmptyWidget(Common.UIString('Nothing to preview'));

    // We support non-strict JSON parsing by parsing an AST tree which is why we offload it to a worker.
    var parsedJSON = await SourceFrame.JSONView.parseJSON(content);
    if (parsedJSON && typeof parsedJSON.data === 'object')
      return SourceFrame.JSONView.createSearchableView(/** @type {!SourceFrame.ParsedJSON} */ (parsedJSON));

    var resourceType = provider.contentType() || Common.resourceTypes.Other;

    // TODO(allada) This should be done without checking provider's type.
    // See: https://chromium-review.googlesource.com/c/chromium/src/+/676625
    var whitelist = new Set(['text/html', 'text/plain', 'application/xhtml+xml']);
    if (provider instanceof SDK.NetworkRequest && whitelist.has(mimeType) &&
        (resourceType === Common.resourceTypes.XHR || resourceType === Common.resourceTypes.Fetch)) {
      var contentData = await provider.contentData();
      var dataURL = Common.ContentProvider.contentAsDataURL(
          contentData.content, mimeType, contentData.encoded, contentData.encoded ? 'utf-8' : null);
      return dataURL ? new SourceFrame.HTMLView(dataURL) : null;
    }

    var parsedXML = SourceFrame.XMLView.parseXML(content, mimeType);
    if (parsedXML)
      return SourceFrame.XMLView.createSearchableView(parsedXML);

    if (resourceType.isTextType()) {
      var highlighterType = mimeType.replace(/;.*/, '');  // remove charset
      return SourceFrame.ResourceSourceFrame.createSearchableView(provider, highlighterType);
    }

    switch (resourceType) {
      case Common.resourceTypes.Image:
        return new SourceFrame.ImageView(mimeType, provider);
      case Common.resourceTypes.Font:
        return new SourceFrame.FontView(mimeType, provider);
    }
    return null;
  }
};