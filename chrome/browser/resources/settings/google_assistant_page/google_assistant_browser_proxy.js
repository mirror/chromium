// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A helper object used from the google assistant section
 * to interact with the browser.
 */

cr.define('settings', function() {
  console.log("setting up assistant browser proxy");
  /** @interface */
  class GoogleAssistantBrowserProxy {
    /**
     * Enables or disables the Google Assistant.
     * @param {boolean} assistantEnabled
     */
    setGoogleAssistantEnabled(assistantEnabled) {}

    /**
     * Enables or disables screen context for the Google Assistant.
     * @param {boolean} contextEnabled
     */
    setGoogleAssistantContextEnabled(contextEnabled) {}

    /**
     * Launches into the Google Assistant app settings.
     */
    launchGoogleAssistantSettings() {}
  }

  /**
   * @implements {settings.GoogleAssistantBrowserProxy}
   */
  class GoogleAssistantBrowserProxyImpl {
    /** @override */
    setGoogleAssistantEnabled(assistantEnabled) {
      chrome.send('setGoogleAssistantEnabled', [assistantEnabled]);
    }

    /** @override */
    setGoogleAssistantContextEnabled(contextEnabled) {
      chrome.send('setGoogleAssistantContextEnabled', [contextEnabled]);
    }

    /** @override */
    showGoogleAssistantSettings() {
      chrome.send('showGoogleAssistantSettings');
    }

  }

  // The singleton instance_ is replaced with a test version of this wrapper
  // during testing.
  cr.addSingletonGetter(GoogleAssistantBrowserProxyImpl);

  return {
    GoogleAssistantBrowserProxy: GoogleAssistantBrowserProxy,
    GoogleAssistantBrowserProxyImpl: GoogleAssistantBrowserProxyImpl,
  };
});
