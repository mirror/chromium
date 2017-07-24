// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Runs the Polymer Accessibility Settings tests. */

/** @const {string} Path to root from chrome/test/data/webui/settings/. */
var ROOT_PATH = '../../../../../';

// Polymer BrowserTest fixture and aXe-core accessibility audit.
GEN_INCLUDE([
  ROOT_PATH + 'chrome/test/data/webui/polymer_browser_test_base.js',
  ROOT_PATH + 'third_party/axe-core/axe.js',
]);

/**
 * Test fixture for Accessibility of Chrome Settings.
 * @constructor
 * @extends {PolymerTest}
 */
function SettingsAccessibilityTest() {}

/**
 * Run aXe-core accessibility audit, print console-friendly representation
 * of violations to console, and fail the test.
 * @param {{rules: {string: {enable: boolean}}} options Dictionary mapping aXe
 *    audit rule names to whether they are enabled. Rules enabled by default.
 * @return {Promise} A promise that will be resolved with the accessibility
 *    audit is complete.
 */
SettingsAccessibilityTest.runAudit = function(options) {
  // Ignore iron-iconset-svg elements that have duplicate ids and result in
  // false postives from the audit.
  var context = {
      exclude: ['iron-iconset-svg']
    };
  var options = options || {};

  return new Promise(function(resolve, reject) {
    axe.run(context, options, function(err, results) {
      if (err) reject(err);

      var violationCount = results.violations.length;
      if (violationCount) {
        // Pretty print out the violations detected by the audit.
        console.log(JSON.stringify(results.violations, null, 4));
        reject('Found ' + violationCount + ' accessibility violations.');
      } else {
        resolve();
      }
    });
  });
};

SettingsAccessibilityTest.prototype = {
  __proto__: PolymerTest.prototype,

  /** @override */
  browsePreload: 'chrome://md-settings/',

  // Include files that define the mocha tests.
  extraLibraries: PolymerTest.getLibraries(ROOT_PATH).concat([
      'ensure_lazy_loaded.js',
      'passwords_and_autofill_fake_data.js',
      'passwords_accessibility_test.js'
  ]),

  // TODO(hcarmona): Remove once ADT is not longer in the testing infrastructure
  runAccessibilityChecks: false,

  setUp: function() {
    PolymerTest.prototype.setUp.call(this);
    settings.ensureLazyLoaded();

    /**
     * Define a basic Mocha suite for testing a route given a specific path for
     * accessibility.
     *
     * @param {!string} name Name of the suite.
     * @param {!string} path Path associated with the route
     */
    var defineSettingsAccessibilityTest = function(name, path) {
      suite(name + "_ROUTE", function() {
        setup(function() {
          settings.navigateTo(settings.routes[name]);
          Polymer.dom.flush();
        });

        test('AccessibleWithNoChanges', function() {
          return SettingsAccessibilityTest.runAudit();
        });
      });
    };

    // Define test for all suites
    Object.keys(settings.routes).forEach(function(route) {
      defineSettingsAccessibilityTest(route, settings.routes[route]);
    });
  },
};

// Run each mocha test in isolation (within a new TEST_F() call). ADVANCED route
// excluded because it only serves as a parent of subpages, and should not be
// possible to navigate to it directly.
// TODO(quacht): Add SIGN_OUT route once test is written.
// TODO(quacht): Add SITE_SETTINGS_DATA_DETAILS route once test is written.

// This route array must be kept in sync with settings.Routes.
// TODO: Investigate a way to avoid hardcoding the routes for creating a TEST_F
// for each route.
var routes = [
  'BASIC',
  'ABOUT',
  'IMPORT_DATA',
  'APPEARANCE',
  'FONTS',
  'DEFAULT_BROWSER',
  'SEARCH',
  'SEARCH_ENGINES',
  'ON_STARTUP',
  'STARTUP_URLS',
  'PEOPLE',
  'SYNC',
  // TODO(quacht): Enable once Polymer bug involving malformed ariadescribedby
  // is resolved.
  // 'MANAGE_PROFILE',,
  'CLEAR_BROWSER_DATA',
  'PRIVACY',
  'CERTIFICATES',
  'SITE_SETTINGS',
  'SITE_SETTINGS_HANDLERS',
  'SITE_SETTINGS_ADS',
  'SITE_SETTINGS_AUTOMATIC_DOWNLOADS',
  'SITE_SETTINGS_BACKGROUND_SYNC',
  'SITE_SETTINGS_CAMERA',
  'SITE_SETTINGS_COOKIES',
  'SITE_SETTINGS_IMAGES',
  'SITE_SETTINGS_JAVASCRIPT',
  'SITE_SETTINGS_LOCATION',
  'SITE_SETTINGS_MICROPHONE',
  'SITE_SETTINGS_NOTIFICATIONS',
  'SITE_SETTINGS_FLASH',
  'SITE_SETTINGS_POPUPS',
  'SITE_SETTINGS_UNSANDBOXED_PLUGINS',
  'SITE_SETTINGS_MIDI_DEVICES',
  'SITE_SETTINGS_USB_DEVICES',
  'SITE_SETTINGS_ZOOM_LEVELS',
  'SITE_SETTINGS_PDF_DOCUMENTS',
  'SITE_SETTINGS_PROTECTED_CONTENT',
  'PASSWORDS',
  'AUTOFILL',
  'MANAGE_PASSWORDS',
  'LANGUAGES',
  // TODO(quacht): Enable once Polymer bug involving malformed ariadescribedby
  // is resolved.
  // 'EDIT_DICTIONARY',
  'DOWNLOADS',
  'PRINTING',
  'CLOUD_PRINTERS',
  'ACCESSIBILITY',
  // TODO(quacht): Enable once this test no longer times out.
  // 'SYSTEM',
  'RESET',
  'RESET_DIALOG',
  'TRIGGERED_RESET_DIALOG'
 ].forEach(function(route) {
  TEST_F('SettingsAccessibilityTest', route, function() {
    mocha.grep(route + "_ROUTE").run();
  });
});
