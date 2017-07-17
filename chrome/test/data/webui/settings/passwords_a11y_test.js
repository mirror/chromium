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
  })

  test('Accessible with 0 passwords', function() {
    assertEquals(passwordsSection.savedPasswords.length, 0);
    return SettingsAccessibilityTest.runAudit();
  });

  test('Accessible with 100 passwords', function() {
    var fakePasswords = [];
    for (var i = 0; i < 100; i++) {
      fakePasswords.push(FakeDataMaker.passwordEntry())
    }

    // Set list of passwords.
    passwordManager.lastCallback.addSavedPasswordListChangedListener(
        fakePasswords);
    Polymer.dom.flush();

    assertEquals(100, passwordsSection.savedPasswords.length);

    return SettingsAccessibilityTest.runAudit();
  });
});