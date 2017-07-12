// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of accessibility tests for the passwords page. */

/**
 * Run aXe-core accessibility audit, print console-friendly representation
 * of violations to console, and fail the test .
 */
function runA11yAudit() {
  return new Promise(function(resolve, reject) {

    var options = {
      // Exclude rules that don't apply since we are injecting the settings-ui
      // into an empty page.
      'rules': {
        'document-title': { enabled: false },
        'html-has-lang': { enabled: false }
      }
    };

    axe.run(document, options, function (err, results) {
      if (err) console.log(err);

      // Pretty print out the violations detected by the audit.
      console.log(JSON.stringify(results.violations, null, 4));
      var violationCount = results.violations.length;

      if (violationCount) {
        reject('Found ' + violationCount + ' accessibility violations.');
      } else {
        resolve();
      }
    });
  })
};

/**
 * Creates PasswordManagerExpectations with the values expected after first
 * creating the element.
 * @return {!PasswordManagerExpectations}
 */
function basePasswordExpectations() {
  var expected = new PasswordManagerExpectations();
  expected.requested.passwords = 1;
  expected.requested.exceptions = 1;
  expected.listening.passwords = 1;
  expected.listening.exceptions = 1;
  return expected;
}

suite('SettingsPasswordsAccessibility', function() {
  var passwordsSection = null;
  var passwordManager = null;

  // Assure that the rest of the page is lazy loaded so that the we can
  // can analyze the password manager, which is lazy loaded because it's part of
  // the advanced settings.
  suiteSetup(function() {
    // Ensure lazy load so that the PassworddManagerImpl can be overwritten and
    // mock data can be supplied to the UI.
    return new Promise(function(resolve, reject) {
      // This URL needs to match the URL passed to <settings-idle-load> from
      // <settings-basic-page>.
      Polymer.Base.importHref('/lazy_load.html', resolve, reject, true);
    });
  });

  setup(function() {
    return new Promise(function(resolve, fail) {
      // Reset the blank to be a blank page.
      PolymerTest.clearBody();

      // Set the URL of the page to be the desire route so that when the
      // settings ui is injected, the correct content will render based on the
      // initialized URL without attaching things, to allow replacement of the
      // data layer with fakes/mocks.
      window.history.pushState('object or string', 'Test', '/passwords');

      PasswordManagerImpl.instance_ = new TestPasswordManager();
      passwordManager = PasswordManagerImpl.instance_

      var settingsUi = document.createElement('settings-ui');
      settingsUi.addEventListener('settings-section-expanded', function () {

        // Passwords section should be loaded.
        passwordsSection = document.querySelector('* /deep/ passwords-section');
        assertTrue(!!passwordsSection);

        assertEquals(
            passwordsSection.passwordManager_, passwordManager);

        // The callback is coming from the manager, so the element shouldn't
        // have additional calls to the manager after the base expectations.
        passwordsSection.passwordManager_.assertExpectations(
            basePasswordExpectations());

        resolve();
      });

      document.body.appendChild(settingsUi);
      Polymer.dom.flush();
    });
  })

  test('Accessible with 0 passwords', function() {
    assertEquals(passwordsSection.savedPasswords.length, 0);
    return runA11yAudit()
  });

  test('Accessible with 100 passwords', function() {
    var fakePasswords = [];
    for (var i = 0; i < 100; i++) {
      fakePasswords.push(FakeDataMaker.passwordEntry())
    }

    // Change the list of passwords to show to be the list of fake passwords.
    passwordManager.lastCallback.addSavedPasswordListChangedListener(
        fakePasswords);
    Polymer.dom.flush();

    // Assert that the number of passwords in the list is equivalent to that of
    // the password entries shown on the page.
    assertEquals(passwordsSection.savedPasswords.length, 100);

    return runA11yAudit();
  });

  test('Accessible with 1000 passwords', function() {
    var fakePasswords = [];
    for (var i = 0; i < 1000; i++) {
      fakePasswords.push(FakeDataMaker.passwordEntry())
    }

    // Change the list of passwords to show to be the list of fake passwords.
   passwordManager.lastCallback.addSavedPasswordListChangedListener(
      fakePasswords);
    Polymer.dom.flush();

    // Assert that the number of passwords in the list is equivalent to that of
    // the password entries shown on the page.
    assertEquals(passwordsSection.savedPasswords.length, 1000)

    return runA11yAudit();
  });
});