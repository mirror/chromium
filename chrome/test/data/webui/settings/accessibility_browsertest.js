// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Runs the Polymer Accessibility Settings tests. */

/** @const {string} Path to root from chrome/test/data/webui/settings/. */
var ROOT_PATH = '../../../../../';

// Polymer BrowserTest fixture.
GEN_INCLUDE([
  ROOT_PATH + 'chrome/test/data/webui/polymer_browser_test_base.js',
  ROOT_PATH + 'third_party/axe-core/axe.js',
]);

/**
 * Test fixture for Accessibility of the BASIC route of Chrome Settings.
 * @constructor
 * @extends {CrSettingsAccessibility}
 */
function CrSettingsA11yPasswordsTest() {}

CrSettingsA11yPasswordsTest.prototype = {
  __proto__: PolymerTest.prototype,

  /** @override */
  browsePreload: 'chrome://md-settings/settings_ui/settings_ui.html',

  extraLibraries: PolymerTest.getLibraries(ROOT_PATH).concat([
      'ensure_lazy_loaded.js',
      'passwords_and_autofill_fake_data.js',
      'passwords_a11y_test.js'
  ]),

  runAccessibilityChecks: false,

  setUp: function() {
    PolymerTest.prototype.setUp.call(this);
  }
};

TEST_F('CrSettingsA11yPasswordsTest', 'All', function() {
  mocha.run();
});

