// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('signin_sync_confirmation', function() {

  suite('SigninSyncConfirmationTest', function() {
    let app;
    setup(function() {
      PolymerTest.clearBody();
      app = document.createElement('sync-confirmation-app');
      document.body.append(app);
    });

    // Tests that no DCHECKS are thrown during initialization of the UI.
    test('LoadPage', function() {
      assertTrue(!!app.$.heading);
    });
  });

  // This test suite verifies that the consent strings recorded in various
  // scenarios are as expected. If the corresponding HTML file was updated
  // without also updating the attributes referring to consent strings,
  // this test will break.
  suite('SigninSyncConfirmationConsentRecordingTest', function() {
    let app;

    setup(function() {
      PolymerTest.clearBody();
      app = document.createElement('sync-confirmation-app');
      document.body.append(app);

      // We don't care about the 'confirm' and 'settings' messages in this test.
      registerMessageCallback('confirm', null, function() {});
      registerMessageCallback('goToSettings', null, function() {});
    });

    teardown(function() {
      registerMessageCallback('confirm', null, undefined);
      registerMessageCallback('goToSettings', null, undefined);
      registerMessageCallback('recordConsent', null, undefined);
    });

    var standardConsentDescriptionText = [
        'Get even more from Chrome',
        'Sync your bookmarks, passwords, and history on all your devices',
        'Get more personalized experiences, such as better ' +
            'content suggestions and smarter Translate',
        'Bring powerful Google services like spell check and tap to search ' +
            'to Chrome',
        'You can customize what information Google collects in Settings ' +
            'anytime.',
        'Google may use your browsing activity, content on some sites you ' +
            'visit, and other browser interactions to personalize Chrome and ' +
            'other Google services like Translate, Search, and ads.',
    ];

    /**
     * Verifies that the 'recordConsent' message was sent with the correct
     * arguments.
     * @param {Object} expected_description The expected value of
     *     the description argument.
     * @param {Object} expected_confirmation The expected value of
     *     the confirmation argument.
     */
    function expectArguments(expected_description, expected_confirmation) {
      // This test makes comparisons with strings in their default locale,
      // which is en-US.
      if (navigator.language != "en-US") {
        console.warn('Cannot verify strings for the ' + navigator.language +
                     'locale.');
        return;
      }

      registerMessageCallback('recordConsent', null,
          function(arguments) {
            assertEquals(2, arguments.length);
            description = arguments[0];
            confirmation = arguments[1];
            assertEquals(JSON.stringify(expected_description),
                         JSON.stringify(description));
            assertEquals(JSON.stringify(expected_confirmation),
                         JSON.stringify(confirmation));
          });
    }

    // Tests that the expected strings are recorded when clicking the Confirm
    // button.
    test('recordConsentOnConfirm', function() {
      expectArguments(standardConsentDescriptionText, 'Yes, I\'m in');
      app.$$('#confirmButton').click();
    });

    // Tests that the expected strings are recorded when clicking the Confirm
    // button.
    test('recordConsentOnSettingsLink', function() {
      expectArguments(standardConsentDescriptionText,
                      'You can customize what information Google collects ' +
                          'in Settings anytime.');
      app.$$('#settingsLink').click();
    });
  });
});
