// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Runs the Polymer Accessibility Settings tests. */

/** @const {string} Path to root from chrome/test/data/webui/settings/. */
var ROOT_PATH = '../../../../../';

// Polymer BrowserTest fixture and aXe-core accessibility audit.
GEN_INCLUDE([
  ROOT_PATH + 'chrome/test/data/webui/polymer_browser_test_base.js',
  ROOT_PATH + 'chrome/test/data/webui/settings/accessibility_audit_rules.js',
  ROOT_PATH + 'chrome/test/data/webui/settings/accessibility_test.js',
  ROOT_PATH + 'third_party/axe-core/axe.js',
]);

/**
 * Test fixture for Accessibility of Chrome Settings.
 * @constructor
 * @extends {PolymerTest}
 */
function SettingsAccessibilityTest() {}

SettingsAccessibilityTest.prototype = {
  __proto__: PolymerTest.prototype,

  /** @override */
  browsePreload: 'chrome://md-settings/',

  // Include files that define the mocha tests.
  extraLibraries: PolymerTest.getLibraries(ROOT_PATH).concat([
    'ensure_lazy_loaded.js',
  ]),

  // TODO(hcarmona): Remove once ADT is not longer in the testing infrastructure
  runAccessibilityChecks: false,

  setUp: function() {
    PolymerTest.prototype.setUp.call(this);
    settings.ensureLazyLoaded();
  },
};

/**
 * Create a GTest unit test belonging into |testFixture| for every audit
 * rule for |route| except for |rulesToSkip|.
 * @param {string} testFixture Name of the test fixture associated with the
 *    tests.
 * @param {!AccessibilityTest.Definition} testDef Object configuring the test.
 */
SettingsAccessibilityTest.createTestF = function(testFixture, testDef) {
  rulesToSkip = testDef.rulesToSkip || [];

  // TODO(hcarmona): remove 'region from rulesToSkip after fixing violations
  // across settings routes.
  rulesToSkip.push('region');

  // Define a unit test for every audit rule except rules to skip.
  AccessibilityTest.ruleIds.forEach(function(ruleId) {
    if (rulesToSkip.indexOf(ruleId) == -1) {
      var testName = testDef.name + '_' + ruleId;
      // Replace hyphens, which break the build.
      var testName = testName.replace(new RegExp('-', 'g'), '_');
      TEST_F(testFixture, testName, function() {
            AccessibilityTest.define(testDef);
            mocha.grep(testDef.name + '_' + ruleId).run();
          });
    }
  });
};