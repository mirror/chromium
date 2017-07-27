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
    'showGoogleAssistantSettings',
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
  showGoogleAssistantSettings: function() {
    this.methodCalled('showGoogleAssistantSettings');
  },
};

var googleAssistantPage = null;

/** @type {?TestGoogleAssistantBrowserProxy} */
var GoogleAssistantBrowserProxy = null;

suite('GoogleAssistantHandler', function() {
  setup(function() {
    GoogleAssistantBrowserProxy = new TestGoogleAssistantBrowserProxy();
    settings.GoogleAssistantBrowserProxyImpl.instance_ =
        GoogleAssistantBrowserProxy;

    PolymerTest.clearBody();

    prefElement = document.createElement('settings-prefs');
    document.body.appendChild(prefElement);

    return CrSettingsPrefs.initialized.then(function() {
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
    assertTrue(!!button);
    assertFalse(button.disabled);
    assertFalse(button.checked);

    // Tap the enable toggle button and ensure the state becomes enabled.
    MockInteractions.tap(button.$.control);
    Polymer.dom.flush();
    assertTrue(button.checked);
    return GoogleAssistantBrowserProxy.whenCalled('setGoogleAssistantEnabled');
  });

  test('toggleAssistant', function() {
    Polymer.dom.flush();
    var button = googleAssistantPage.$$('#googleAssistantEnable');
    assertTrue(!!button);
    assertFalse(button.disabled);
    assertFalse(button.checked);

    // Tap the enable toggle button and ensure the state becomes enabled.
    MockInteractions.tap(button.$.control);
    Polymer.dom.flush();
    assertTrue(button.checked);
    return GoogleAssistantBrowserProxy.whenCalled('setGoogleAssistantEnabled');
  });

  test('toggleAssistantContext', function() {
    var button = googleAssistantPage.$$('#googleAssistantContextEnable');
    assertFalse(!!button);
    googleAssistantPage.setPrefValue(
        "settings.voice_interaction.enabled", true);
    Polymer.dom.flush();
    button = googleAssistantPage.$$('#googleAssistantContextEnable');
    assertTrue(!!button);
    assertFalse(button.disabled);
    assertFalse(button.checked);

    // Tap the enable toggle button and ensure the state becomes enabled.
    MockInteractions.tap(button.$.control);
    Polymer.dom.flush();
    assertTrue(button.checked);
    return GoogleAssistantBrowserProxy.whenCalled(
        'setGoogleAssistantContextEnabled');
  });

  test('tapOnAssistantSettings', function() {
    var button = googleAssistantPage.$$('#googleAssistantSettings');
    assertFalse(!!button);
    googleAssistantPage.setPrefValue(
        "settings.voice_interaction.enabled", true);
    Polymer.dom.flush();
    button = googleAssistantPage.$$('#googleAssistantSettings');
    assertTrue(!!button);

    // Tap the enable toggle button and ensure the state becomes enabled.
    MockInteractions.tap(button);
    Polymer.dom.flush();
    return GoogleAssistantBrowserProxy.whenCalled(
        'showGoogleAssistantSettings');
  });

});
