// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Runs the Polymer Accessibility Settings tests. */

// Disable in debug and memory sanitizer modes because of timeouts.
GEN('#if defined(NDEBUG)');

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
    'passwords_and_autofill_fake_data.js',
    'passwords_a11y_test.js',
    'basic_a11y_test.js',
  ]),

  // TODO(hcarmona): Remove once ADT is not longer in the testing infrastructure
  runAccessibilityChecks: false,

  setUp: function() {
    PolymerTest.prototype.setUp.call(this);
    settings.ensureLazyLoaded();
  },
};

/**
 * Create a GTest unit test belonging into |testFixtureName| for every audit
 * rule for |route| except for |rulesToSkip|.
 * @param {string} testFixtureName Name of the test fixture associated with the
 *    tests.
 * @param {string} route Name of the page to test.
 * @param {?Array<string>} rulesToSkip List of audit rule IDs to not create
 *    tests for.
 */
function createTestF(testFixtureName, route, rulesToSkip) {
  rulesToSkip = rulesToSkip || [];

  // TODO(hcarmona): remove 'region from rulesToSkip after fixing violation.
  rulesToSkip.push('region')

  // Define a unit test for every audit rule except rules to skip.
  AccessibilityTest.ruleIds.forEach(function(ruleId) {
    if (rulesToSkip.indexOf(ruleId) == -1) {
      // Replace hyphens, which break the build.
      var ruleName = ruleId.replace(new RegExp('-', 'g'), '_');
      TEST_F(
          testFixtureName, route + '_' + ruleName,
          function() {
            mocha.grep(route + '_' + ruleId).run();
          });
    }
  });
};

/**
 * Test fixture for MANAGE PASSWORDS
 * @constructor
 * @extends {PolymerTest}
 */
function SettingsA11yManagePasswords() {}

SettingsA11yManagePasswords.prototype = {
  __proto__: SettingsAccessibilityTest.prototype,

  // Include files that define the mocha tests.
  extraLibraries: PolymerTest.getLibraries(ROOT_PATH).concat([
    'ensure_lazy_loaded.js',
    'passwords_and_autofill_fake_data.js',
    'passwords_a11y_test.js',
  ]),
};

// On this page, there are few tab stops before the main content.
var rulesToSkip = ['skip-link'];
// Disable rules flaky for CFI build.
rulesToSkip.concat(['meta-viewpoint', 'list', 'frame-title', 'label',
    'hidden_content', 'aria-valid-attr-value', 'button-name']);
createTestF('SettingsA11yManagePasswords', 'MANAGE_PASSWORDS', rulesToSkip);

/**
 * Test fixture for BASIC
 * @constructor
 * @extends {PolymerTest}
 */
function SettingsA11yBasic() {}

SettingsA11yBasic.prototype = {
  __proto__: SettingsAccessibilityTest.prototype,

  // Include files that define the mocha tests.
  extraLibraries: PolymerTest.getLibraries(ROOT_PATH).concat([
    'ensure_lazy_loaded.js',
    'basic_a11y_test.js',
  ]),
};
createTestF('SettingsA11yBasic', 'BASIC', ['skip-link']);

/**
 * Test fixture for ABOUT
 * @constructor
 * @extends {PolymerTest}
 */
function SettingsA11yAbout() {}

SettingsA11yAbout.prototype = {
  __proto__: SettingsAccessibilityTest.prototype,

  // Include files that define the mocha tests.
  extraLibraries: PolymerTest.getLibraries(ROOT_PATH).concat([
    'ensure_lazy_loaded.js',
    'about_a11y_test.js',
  ]),
};
createTestF('SettingsA11yAbout', 'ABOUT', ['skip-link']);

GEN('#endif  // defined(NDEBUG)');


