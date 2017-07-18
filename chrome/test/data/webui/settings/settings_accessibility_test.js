// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of accessibility tests for the passwords page. */

/**
 * Define a basic Mocha suite for testing a route given a specific path for
 * accessibility.
 *
 * @param {!String} name Name of the suite.
 * @param {!String} path Path associated with the route
 * @param {Boolean} opt_waitForExpansion True if need to wait for settings
 * section to expand.
 */
function defineSuite(name, path, opt_waitForExpansion=false) {
  suite(name, function() {

    // Only return a promise in the set up if settings section will expand when
    // loading the page.
    if (opt_waitForExpansion) {
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
    } else {
      setup(function() {
        // Reset the blank to be a blank page.
        PolymerTest.clearBody();

        // Set the URL of the page to render to load the correct view upon
        // injecting settings-ui without attaching listeners.
        window.history.pushState('object or string', 'Test', path);

        var settingsUi = document.createElement('settings-ui');
        document.body.appendChild(settingsUi);
      });
    }

    test('AccessibleWithNoChanges', function() {
      return SettingsAccessibilityTest.runAudit();
    });
  });
}

var testsWithExpansion = ['SEARCH_ENGINES', 'FONTS', 'CLOUD_PRINTERS'];
var separatelyDefined = ['PASSWORDS', 'SIGN_OUT'];

// Work with map of routes since using the settings.routes requires waiting for
// it to be set. When this code get executed, settings.router is undefined.
var routeMap = {
  'BASIC': '/',
  'ABOUT': '/help',
  'IMPORT_DATA': '/importData',
  'SIGN_OUT': '/signOut',
  'APPEARANCE': '/appearance',
  'FONTS': '/fonts',
  'DEFAULT_BROWSER': '/defaultBrowser',
  'SEARCH': '/search',
  'SEARCH_ENGINES': '/searchEngines',
  'ON_STARTUP': '/onStartup',
  'STARTUP_URLS': '/startupUrls',
  'PEOPLE': '/people',
  'SYNC': '/syncSetup',
  'MANAGE_PROFILE': '/manageProfile',
  'ADVANCED': '/advanced',
  'CLEAR_BROWSER_DATA': '/clearBrowserData',
  'PRIVACY': '/privacy',
  'CERTIFICATES': '/certificates',
  'SITE_SETTINGS': '/content',
  'SITE_SETTINGS_HANDLERS': '/handlers',
  'SITE_SETTINGS_ADS': '/content/ads',
  'SITE_SETTINGS_AUTOMATIC_DOWNLOADS': '/content/automaticDownloads',
  'SITE_SETTINGS_BACKGROUND_SYNC': '/content/backgroundSync',
  'SITE_SETTINGS_CAMERA': '/content/camera',
  'SITE_SETTINGS_COOKIES': '/content/cookies',
  'SITE_SETTINGS_DATA_DETAILS': '/cookies/detail',
  'SITE_SETTINGS_IMAGES': '/content/images',
  'SITE_SETTINGS_JAVASCRIPT': '/content/javascript',
  'SITE_SETTINGS_LOCATION': '/content/location',
  'SITE_SETTINGS_MICROPHONE': '/content/microphone',
  'SITE_SETTINGS_NOTIFICATIONS': '/content/notifications',
  'SITE_SETTINGS_FLASH': '/content/flash',
  'SITE_SETTINGS_POPUPS': '/content/popups',
  'SITE_SETTINGS_UNSANDBOXED_PLUGINS': '/content/unsandboxedPlugins',
  'SITE_SETTINGS_MIDI_DEVICES': '/content/midiDevices',
  'SITE_SETTINGS_USB_DEVICES': '/content/usbDevices',
  'SITE_SETTINGS_ZOOM_LEVELS': '/content/zoomLevels',
  'SITE_SETTINGS_PDF_DOCUMENTS': '/content/pdfDocuments',
  'SITE_SETTINGS_PROTECTED_CONTENT': '/content/protectedContent',
  'PASSWORDS': '/passwordsAndForms',
  'AUTOFILL': '/autofill',
  'MANAGE_PASSWORDS': '/passwords',
  'LANGUAGES': '/languages',
  'EDIT_DICTIONARY': '/editDictionary',
  'DOWNLOADS': '/downloads',
  'PRINTING': '/printing',
  'CLOUD_PRINTERS': '/cloudPrinters',
  'ACCESSIBILITY': '/accessibility',
  'SYSTEM': '/system',
  'RESET': '/reset',
  'RESET_DIALOG': '/resetProfileSettings',
  'TRIGGERED_RESET_DIALOG': '/triggeredResetProfileSettings'
};

// Define test for all suites
Object.keys(routeMap).forEach(function(route) {
  if (separatelyDefined.indexOf(route)) {
    return;
  }

  if (testsWithExpansion.indexOf(route) != -1) {
    defineSuite(route, routeMap[route], true);
  } else {
  defineSuite(route, routeMap[route]);
  }
});

suite('PASSWORDS', function() {
  var passwordsSection = null;
  var passwordManager = null;

  //TODO(hcarmona): Remove duplicate iron-icons so that they do not violate the
  // duplicate ids audit rule.
  var auditOptions = {
    context: {
      exclude: ['iron-iconset-svg']
    }
  };

  setup(function() {
    return new Promise(function(resolve, fail) {
      // Reset the blank to be a blank page.
      PolymerTest.clearBody();

      // Set the URL of the page to render to load the correct view upon
      // injecting settings-ui without attaching listeners.
      window.history.pushState('object or string', 'Test', '/passwords');

      PasswordManagerImpl.instance_ = new TestPasswordManager();
      passwordManager = PasswordManagerImpl.instance_;

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
    return SettingsAccessibilityTest.runAudit(auditOptions);
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
    return SettingsAccessibilityTest.runAudit(auditOptions);
  });
});
