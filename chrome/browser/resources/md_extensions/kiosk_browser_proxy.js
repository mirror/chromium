// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A helper object used from the "Kiosk" dialog to interact with
 * the browser.
 */
cr.define('extensions', function() {

  /** @interface */
  class KioskBrowserProxy {
    /**
     * @param {string} appId 
     * @return {!Promise<!ChannelInfo>}
     */
    addKioskApp(appId) {}

    /**
     * @param {string} appId 
     * @return {!Promise<!ChannelInfo>}
     */
    disableKioskAutoLaunch(appId) {}

    /**
     * @param {string} appId 
     * @return {!Promise<!ChannelInfo>}
     */
    enableKioskAutoLaunch(appId) {}
    
    /**
     * @param {string} appId 
     * @return {!Promise<!ChannelInfo>}
     */
    removeKioskApp(appId) {}

    /**
     * @param {bool} disable 
     * @return {!Promise<!ChannelInfo>}
     */
    setDisableBailoutShortcut(disable) {}
  }

  /**
   * @implements {settings.KioskBrowserProxy}
   */
  class KioskBrowserProxyImpl {
    /** @override */
    addKioskApp(appId) {
      //return chrome.sendWithPromise('addKioskApp', disable);
      console.log('chrome.sendWithPromise(addKioskApp, ' + appId + ')');
      return Promise.resolve();
    }

    /** @override */
    disableKioskAutoLaunch(appId) {
      //return chrome.sendWithPromise('disableKioskAutoLaunch', disable);
      console.log('chrome.sendWithPromise(disableKioskAutoLaunch, ' + appId + ')');
      return Promise.resolve();
    }
    
    /** @override */
    enableKioskAutoLaunch(appId) {
      //return chrome.sendWithPromise('enableKioskAutoLaunch', disable);
      console.log('chrome.sendWithPromise(enableKioskAutoLaunch, ' + appId + ')');
      return Promise.resolve();
    }

    /** @override */
    removeKioskApp(appId) {
      //return chrome.sendWithPromise('removeKioskApp', disable);
      console.log('chrome.sendWithPromise(removeKioskApp, ' + appId + ')');
      return Promise.resolve();
    }

    /** @override */
    setDisableBailoutShortcut(disable) {
      //return chrome.sendWithPromise('setDisableBailoutShortcut', disable);
      console.log('chrome.sendWithPromise(setDisableBailoutShortcut, ' + disable + ')');
      return Promise.resolve();
    }
  }

  cr.addSingletonGetter(KioskBrowserProxyImpl);

  return {
    KioskBrowserProxy: KioskBrowserProxy,
    KioskBrowserProxyImpl: KioskBrowserProxyImpl,
  };
});
