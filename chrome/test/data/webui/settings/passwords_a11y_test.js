// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Define accessibility tests for the MANAGE_PASSWORDS route.
 */

AccessibilityTest.define({
  /** @override */
  name: 'MANAGE_PASSWORDS',
  /** @type {PasswordManager} */
  passwordManager: null,
  /** @type {PasswordsSectionElement}*/
  passwordsSection: null,
  /** @override */
  setup: function() {
    return new Promise(function(resolve) {
      // Reset to a blank page.
      PolymerTest.clearBody();

      // Set the URL to be that of specific route to load upon injecting
      // settings-ui. Simply calling settings.navigateTo(route) prevents
      // use of mock APIs for fake data.
      window.history.pushState(
          'object or string', 'Test', settings.routes.MANAGE_PASSWORDS.path);

      PasswordManagerImpl.instance_ = new TestPasswordManager();
      passwordManager = PasswordManagerImpl.instance_;

      var settingsUi = document.createElement('settings-ui');

      // The settings section will expand to load the MANAGE_PASSWORDS route
      // (based on the URL set above) once the settings-ui element is attached
      settingsUi.addEventListener('settings-section-expanded', function() {
        // Passwords section should be loaded before setup is complete.
        passwordsSection = settingsUi.$$('settings-main')
                               .$$('settings-basic-page')
                               .$$('settings-passwords-and-forms-page')
                               .$$('passwords-section');
        assertTrue(!!passwordsSection);

        assertEquals(passwordManager, passwordsSection.passwordManager_);

        resolve();
      });

      document.body.appendChild(settingsUi);
    });
  },

  /** @override */
  tests: {
    'Accessible with 0 passwords': function() {
      return new Promise(function(resolve) {
        assertEquals(0, this.passwordsSection.savedPasswords.length);
        resolve();
      });
    },
    'Accessible with 10 passwords': function() {
      return new Promise(function(resolve) {
        var fakePasswords = [];
        for (var i = 0; i < 10; i++) {
          fakePasswords.push(FakeDataMaker.passwordEntry());
        }

        // Set list of passwords.
        this.passwordManager.lastCallback.addSavedPasswordListChangedListener(
            fakePasswords);
        Polymer.dom.flush();

        assertEquals(10, this.passwordsSection.savedPasswords.length);
        resolve();
      });
    },
  },
  exceptions: [
    // TODO(quacht): remove this exception once the color contrast issue is
    // solved.
    // http://crbug.com/748608
    AccessibilityTest.Exception(
        'color-contrast',
        function(nodeResult) {
          var node = nodeResult.element;
          return !(node.id == 'prompt');
        }),
    AccessibilityTest.Exception(
        'aria-valid-attr',
        function(nodeResult) {
          var node = nodeResult.element;
          return !(node.hasAttribute('aria-active-attribute'));
        }),
  ],
});
