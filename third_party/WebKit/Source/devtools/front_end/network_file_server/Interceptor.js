// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

NetworkFileServer.Interceptor = class extends SDK.RequestInterceptor {
  constructor() {
    var interceptionEnabledSetting = Common.moduleSetting('network_file_server.interception-enabled');
    var interceptionFilesMapSetting = Common.settings.createSetting('network_file_server.interception-files-map', {});
    super(interceptionEnabledSetting.get(), new Set(Object.keys(interceptionFilesMapSetting.get())));
    this._interceptionFilesMapSetting = interceptionFilesMapSetting;
    this._interceptionEnabledSetting = interceptionEnabledSetting;
    this._interceptionFilesMapSetting.addChangeListener(
        () => this.setPatterns(new Set(Object.keys(this._interceptionFilesMapSetting.get()))), this);
    this._interceptionEnabledSetting.addChangeListener(
        () => this.setEnabled(this._interceptionEnabledSetting.get()), this);
  }

  /**
   * @override
   * @param {!SDK.InterceptedRequest} interceptedRequest
   * @return {!Promise}
   */
  async handle(interceptedRequest) {
    var mappings = this._interceptionFilesMapSetting.get();
    var url = interceptedRequest.request.url;
    if (!(url in mappings))
      return;
    return new Promise(resolve => {
      var fileURL = 'file://' + mappings[url];
      Host.ResourceLoader.load(fileURL, {}, (statusCode, headers, content) => {
        if (statusCode === 200) {
          var mimeType = Common.ResourceType.mimeFromURL(url) || 'text/x-unknown';
          interceptedRequest.continueRequestWithContent(new Blob([content], {type: mimeType}));
        }
      });
    });
  }
};
