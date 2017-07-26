// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 * @implements {settings.GoogleAssistantBrowserProxy}
 * @extends {TestBrowserProxy}
 */
var TestGoogleAssistantBrowserProxy = function() {
  TestBrowserProxy.call(this, [
    'setGoogleAssistantEnabled',
    'setGoogleAssistantContextEnabled',
    'launchGoogleAssistantSettings',
  ]);
};

TestGoogleAssistantBrowserProxy.prototype = {
  __proto__: TestBrowserProxy.prototype,

  /** @override */
  setGoogleAssistantEnabled: function(enabled) {
    this.methodCalled('setGoogleAssistantEnabled');
  },

  /** @override */
  setGoogleAssistantContextEnabled: function(enabled) {
    this.methodCalled('setGoogleAssistantContextEnabled');
  },

  /** @override */
  launchGoogleAssistantSettings: function() {
    this.methodCalled('launchGoogleAssistantSettings');
  },
};

var googleAssistantPage = null;

/** @type {?TestGoogleAssistantBrowserProxy} */
var GoogleAssistantBrowserProxy = null;

suite('GoogleAssistantHandler', function() {
  setup(function() {
    GoogleAssistantBrowserProxy = new TestGoogleAssistantBrowserProxy();
    settings.GoogleAssistantBrowserProxyImpl.instance_ = GoogleAssistantBrowserProxy;

    PolymerTest.clearBody();

    //CrSettingsPrefs.deferInitialization = true;

    prefElement = document.createElement('settings-prefs');
    document.body.appendChild(prefElement);
    console.log(prefElement);
    console.log(prefElement.outerHTML);
    console.log(prefElement.innerHTML);

    console.log("outside");
    console.log(CrSettingsPrefs.initialized);
    console.log(CrSettingsPrefs.initialized.then);
    return CrSettingsPrefs.initialized.then(function() {
      console.log("inside");
      googleAssistantPage = document.createElement(
        'settings-google-assistant-page');
      googleAssistantPage.prefs = prefElement.prefs;
      document.body.appendChild(googleAssistantPage);
    });
  });

  teardown(function() {
    googleAssistantPage.remove();
  });

  test('toggleAssistant', function() {
    Polymer.dom.flush();
    var button = googleAssistantPage.$$('#googleAssistantEnable');
    console.log(button.outerHTML);
    console.log(button.innerHTML);
    assertTrue(!!button);
    assertFalse(button.disabled);
    assertFalse(button.checked);

    console.log("hello");
    // Tap the enable toggle button and ensure the state becomes enabled.
    MockInteractions.tap(button.$.control);
    Polymer.dom.flush();
    assertTrue(button.checked);
    return GoogleAssistantBrowserProxy.whenCalled('setGoogleAssistantContextEnabled');
  });
});
