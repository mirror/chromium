// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of accessibility tests for the passwords page. */

/**
 * Run aXe-core accessibility audit, print console-friendly representation
 * of violations to console. Fail the test after violations are revealed.
 */
function runA11yAudit() {
  // Run the accessibility audit.
  var auditPromise = new Promise(function(resolve) {
    axe.run(function (err, results) {
      if (err) throw err;
      resolve(results.violations);
    });
  });

  auditPromise.then((violations) => {
    // Pretty print out the violations detected by the audit.
    console.log(JSON.stringify(violations, null, 4));
    console.log('Number of Accessibility Violations = ' + violations.length)
    assert(violations.length == 0);
  }).then(testDone, function(err) {
    testDone([false, 'Found accessibility violations.']);
  });
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

// TODO(quacht): Should I use PolymerTest.clearBody?
var clearBody = function() {
  // Save the div where vulcanize inlines content before clearing the page.
  var vulcanizeDiv = document.querySelector('body > div[hidden][by-vulcanize]');
  document.body.innerHTML = '';
  if (vulcanizeDiv)
    document.body.appendChild(vulcanizeDiv);
};

suite('CrSettingsPasswordsAccessibility', function() {
  // The passwords section element.
  var passwordsSection = null;

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
      clearBody();

      // Set the URL of the page to be the desire route so that when the
      // settings ui is injected, the correct content will render based on the
      // initialized URL.
      // TODO(quacht): determine whether or not this should actually be using
      // settings.navigateTo(desiredRoute)
      window.history.pushState('object or string', 'Test', '/passwords');

      // Use a mock password manager we have control over.
      PasswordManagerImpl.instance_ = new TestPasswordManager();

      // Attach an event listener to the settings ui to handle when the settings
      // ui has expanded.
      settingsUi = document.createElement('settings-ui');
      settingsUi.addEventListener('settings-section-expanded', function (e) {

        // Passwords section should be loaded and should not be null.
        passwordsSection = document.querySelector('* /deep/ passwords-section');
        assert(passwordsSection != null);

        // Make sure that the passwordManager actually belongs to the
        // passwordSectionElement, e.g. they point to the same spot in memory.
        assert(passwordsSection.passwordManager_ ===
            PasswordManagerImpl.instance_);

        // The callback is coming from the manager, so the element shouldn't
        // have additional calls to the manager after the base expectations.
        passwordsSection.passwordManager_.assertExpectations(
            basePasswordExpectations());

        // Once the password manager is verified, the tests may run.
        resolve();
      }, false);

      // Attach the settings to the page.
      document.body.appendChild(settingsUi);

      // Flush to ensure lazy loading by Polymer.
      Polymer.dom.flush();
    });
  })

  test('Accessible with 0 passwords', function() {
    assert(passwordsSection.savedPasswords.length == 0);
    // Run the a11y audit and fail the test if there are any
    // a11y violations.
    runA11yAudit();
  });

  test('Accessible with 100 passwords', function() {
    // Create a list of passwords.
    var fakePasswords = [];
    for (var i = 0; i < 100; i++) {
      fakePasswords.push(FakeDataMaker.passwordEntry())
    }

    // Change the list of passwords to show to be the list of fake passwords.
    PasswordManagerImpl.instance_.lastCallback.addSavedPasswordListChangedListener(fakePasswords);
    Polymer.dom.flush();

    // Assert that the number of passwords in the list is equivalent to that of
    // the password entries shown on the page.
    assert(passwordsSection.savedPasswords.length == 100);
    runA11yAudit();
  });

  test('Accessible with 1000 passwords', function() {
    // Create a list of passwords.
    var fakePasswords = [];
    for (var i = 0; i < 1000; i++) {
      fakePasswords.push(FakeDataMaker.passwordEntry())
    }

    // Change the list of passwords to show to be the list of fake passwords.
    PasswordManagerImpl.instance_.lastCallback.addSavedPasswordListChangedListener(fakePasswords);
    Polymer.dom.flush();

    // Assert that the number of passwords in the list is equivalent to that of
    // the password entries shown on the page.
    assert(passwordsSection.savedPasswords.length == 1000)
    runA11yAudit();
  });
});