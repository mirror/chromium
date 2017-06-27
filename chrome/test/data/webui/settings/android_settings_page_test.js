// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @type {?SettingsSettingsSectionElement} */
var androidSettingsSection = null;

/** @type {?SettingsAndroidSettingsPageElement} */
var androidSettingsPage = null;

/** @type {?TestAndroidAppsBrowserProxy} */
var androidAppsBrowserProxy = null;

suite('AndroidSettingsPageTests', function() {
  setup(function() {
    androidAppsBrowserProxy = new TestAndroidAppsBrowserProxy();
    settings.AndroidAppsBrowserProxyImpl.instance_ = androidAppsBrowserProxy;
    PolymerTest.clearBody();
    androidSettingsSection = document.createElement('settings-section');
    androidSettingsPage = document.createElement(
        'settings-android-settings-page');
    document.body.appendChild(androidSettingsSection);
    androidSettingsSection.appendChild(androidSettingsPage);
    testing.Test.disableAnimationsAndTransitions();
  });

  teardown(function() {
    androidSettingsSection.remove();
  });

  suite('Main', function() {
    setup(function() {
      Polymer.dom.flush();

      return androidAppsBrowserProxy.whenCalled('requestAndroidAppsInfo')
          .then(function() {
            androidAppsBrowserProxy.setAndroidAppsState(false, false);
          });
    });

    test('StateUpdated', function() {
      Polymer.dom.flush();
      assertTrue(!!androidSettingsPage.$$('#manageApps'));
      assertTrue(androidSettingsSection.hidden);

      androidAppsBrowserProxy.setAndroidAppsState(true, true);
      Polymer.dom.flush();
      assertFalse(androidSettingsSection.hidden);
    });

    test('OpenReqest', function() {
      androidAppsBrowserProxy.setAndroidAppsState(true, true);
      Polymer.dom.flush();
      var button = androidSettingsPage.$$('#manageApps');
      assertTrue(!!button);
      var promise = androidAppsBrowserProxy.whenCalled(
          'showAndroidAppsSettings');
      MockInteractions.tap(button);
      Polymer.dom.flush();
      return promise;
    });
  });
});
