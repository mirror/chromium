// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview A helper object used for Internet page. */
cr.exportPath('settings');

cr.define('settings', function() {
  /** @interface */
  class InternetPageBrowserProxy {
    /**
     * Requests Chrome to send list of Arc VPN providers.
     */
    requestArcVpnProviders() {}

    /**
     *  Shows configuration of connnected Arc VPN network.
     *  @param {string} guid
     */
    showNetworkConfigure(guid) {}

    /**
     * Sends add VPN request to extension VPN provider or Arc VPN provider.
     * @param {string} networkType
     * @param {string} appId
     */
    addThirdPartyVpn(networkType, appId) {}
  }

  /**
   * @implements {settings.InternetPageBrowserProxy}
   */
  class InternetPageBrowserProxyImpl {
    /** @override */
    requestArcVpnProviders() {
      chrome.send('requestArcVpnProviders');
    }

    /** @override */
    showNetworkConfigure(guid) {
      chrome.send('configureNetwork', [guid]);
    }

    /** @override */
    addThirdPartyVpn(networkType, appId) {
      chrome.send('addNetwork', [networkType, appId]);
    }
  }

  cr.addSingletonGetter(InternetPageBrowserProxyImpl);

  return {
    InternetPageBrowserProxy: InternetPageBrowserProxy,
    InternetPageBrowserProxyImpl: InternetPageBrowserProxyImpl,
  };
});
