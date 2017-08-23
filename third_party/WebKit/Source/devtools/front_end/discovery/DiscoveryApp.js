// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @implements {Common.App}
 */
Discovery.DiscoveryApp = class {
  constructor() {
    this._rootView = new UI.RootView();
    this._discoveryView = new Discovery.DiscoveryView(this._showInspectorView.bind(this));
  }

  /**
   * @override
   * @param {!Document} document
   */
  presentUI(document) {
    this._discoveryView.show(this._rootView.element);
    this._rootView.attachToDocument(document);
    this._rootView.focus();
  }

  _showDiscoveryView() {
    UI.inspectorView.detach();
    this._discoveryView.show(this._rootView.element);
  }

  _showInspectorView() {
    this._discoveryView.detach();
    UI.inspectorView.show(this._rootView.element);
  }
};

/**
 * @implements {Common.AppProvider}
 */
Discovery.DiscoveryApp.Provider = class {
  /**
   * @override
   * @return {!Common.App}
   */
  createApp() {
    return self.singleton(Discovery.DiscoveryApp);
  }
};

/**
 * @implements {UI.ActionDelegate}
 */
Discovery.DiscoveryApp.ActionDelegate = class {
  /**
   * @override
   * @param {!UI.Context} context
   * @param {string} actionId
   * @return {boolean}
   */
  handleAction(context, actionId) {
    switch (actionId) {
      case 'discovery.show-discovery-view':
        self.singleton(Discovery.DiscoveryApp)._showDiscoveryView();
        return true;
    }
    return false;
  }
};
