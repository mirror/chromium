// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/*
 * @fileoverview Suite of accessibility tests for the SIGN_OUT route.
 */

suite('SIGN_OUT', function() {
  var peoplePage = null;
  var browserProxy = null;
  var profileInfoBrowserProxy = null;

  suiteSetup(function() {
    // Force easy unlock off. Those have their own ChromeOS-only tests.
    loadTimeData.overrideValues({
      easyUnlockAllowed: false,
    });
  });

  setup(function() {
    // Reset the blank to be a blank page.
    PolymerTest.clearBody();

    // Set the URL of the page to render to load the correct view upon
    // injecting settings-ui without attaching listeners.
    window.history.pushState('object or string', 'Test', '/people');

    browserProxy = new TestSyncBrowserProxy();
    settings.SyncBrowserProxyImpl.instance_ = browserProxy;

    profileInfoBrowserProxy = new TestProfileInfoBrowserProxy();
    settings.ProfileInfoBrowserProxyImpl.instance_ = profileInfoBrowserProxy;

    var settingsUi = document.createElement('settings-ui');
    document.body.appendChild(settingsUi);
    Polymer.dom.flush();

    peoplePage = document.querySelector('* /deep/ settings-people-page');
    assert(!!peoplePage);
  });

  test('Accessible Sign Out Dialog', function() {
    var disconnectButton = null;
    return browserProxy.getSyncStatus()
        .then(function(syncStatus) {
          // Navigate to the sign out dialog.
          Polymer.dom.flush();
          disconnectButton = peoplePage.$$('#disconnectButton');

          // TODO(quacht): Figure out why this assertion fails in ChromeOS
          assert(!!disconnectButton);
          MockInteractions.tap(disconnectButton);
          Polymer.dom.flush();
        })
        .then(function() {
          return SettingsAccessibilityTest.runAudit();
        });
  });
});
