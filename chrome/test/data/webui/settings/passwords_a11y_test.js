// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of accessibility tests for the passwords page. */

suite('SettingsPasswordsAccessibility', function() {
  var passwordsSection = null;
  var passwordManager = null;

  setup(function() {
    return new Promise(function(resolve, fail) {
      // Reset the blank to be a blank page.
      PolymerTest.clearBody();

      // Set the URL of the page to render to load the correct view upon
      // injecting settings-ui without attaching listeners.
      window.history.pushState('object or string', 'Test', '/passwords');

      PasswordManagerImpl.instance_ = new TestPasswordManager();
      passwordManager = PasswordManagerImpl.instance_

      var settingsUi = document.createElement('settings-ui');
      settingsUi.addEventListener('settings-section-expanded', function () {

        // Passwords section should be loaded.
        passwordsSection = document.querySelector('* /deep/ passwords-section');
        assertTrue(!!passwordsSection);

        assertEquals(
            passwordManager, passwordsSection.passwordManager_);

        resolve();
      });

      document.body.appendChild(settingsUi);
    });
  });

  test('Accessible with 0 passwords', function() {
    assertEquals(passwordsSection.savedPasswords.length, 0);
    return SettingsAccessibilityTest.runAudit();
  });

  test('Accessible with 100 passwords', function() {
    var fakePasswords = [];
    for (var i = 0; i < 100; i++) {
      fakePasswords.push(FakeDataMaker.passwordEntry());
    }

    // Set list of passwords.
    passwordManager.lastCallback.addSavedPasswordListChangedListener(
        fakePasswords);
    Polymer.dom.flush();

    assertEquals(100, passwordsSection.savedPasswords.length);

    return SettingsAccessibilityTest.runAudit();
  });
});

/**
 * Define a basic Mocha suite for testing a route given a specific path for
 * accessibility.
 *
 * @param name Name of the suite.
 * @param path Path associated with the route
 */
function defineSuite(name, path) {
  suite(name, function() {
    setup(function() {
      return new Promise(function(resolve, fail) {
        // Reset the blank to be a blank page.
        PolymerTest.clearBody();

        // Set the URL of the page to render to load the correct view upon
        // injecting settings-ui without attaching listeners.
        window.history.pushState('object or string', 'Test', path);

        var settingsUi = document.createElement('settings-ui');
        settingsUi.addEventListener('settings-section-expanded', resolve);
        document.body.appendChild(settingsUi);
      });
    });

    test('AccessibleWithNoChanges', function() {
      return SettingsAccessibilityTest.runAudit();
    });
  });
}


// Map test suite names to the route tested by the suite.
// TODO(quacht): Is there a better way? --> this can be programmatically
// generated from the routes object.
var settingsAccessibilityTests = {
  'SettingsFontsAccessibility': '/fonts',
  'SettingsSearchEnginesAccessibility': '/searchEngines',
  'SettingsManageProfileAccessibility': '/manageProfile',
  'SettingsHelpAccessibility': '/help',
  'SettingsImportDataAccessibility': '/importData',
  'SettingsSignOutAccessibility': '/signOut',
  'SettingsClearBrowsingAccessibility': '/clearBrowserData',
  'SettingsResetProfileAccessibility': '/resetProfile',
  'SettingsAppearanceAccessibility': '/appearance',
  'SettingsDefaultBrowserAccessibility': '/defaultBrowser',
  'SettingsSearchAccessibility': '/search',
  'SettingsStartupAccessibility': '/onStartup',
  'SettingsPeopleAccessibility': '/people',
  'SettingsLanguagesAccessibility': '/languages',
  'SettingsEditDictionaryAccessibility': '/editDictionary',
  'SettingsDownloadsAccessibility': '/downloads',
  'SettingsPrintingAccessibility': '/printing',
  'SettingsCloudPrintersAccessibility': '/cloudPrinters',
  'SettingsSyncAccessibility': '/syncSetup',
};

Object.keys(settingsAccessibilityTests).forEach(function(test) {
    var path = settingsAccessibilityTests[test];
    defineSuite(test, path);
});