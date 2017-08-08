// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Define accessibility tests for the BASIC route.
 */

 // Disable in debug and memory sanitizer modes because of timeouts.
GEN('#if defined(NDEBUG)');

/** @const {string} Path to root from chrome/test/data/webui/settings/. */
var ROOT_PATH = '../../../../../';

// Polymer BrowserTest fixture and aXe-core accessibility audit.
GEN_INCLUDE([
  ROOT_PATH + 'chrome/test/data/webui/settings/accessibility_browsertest.js',
]);

/** @type {AccessibilityTest.Definition} */
var testDef = {
  /** @override */
  name: 'BASIC',

  /** @override */
  tests: {
    'Accessible with No Changes': function() {}
  },
  violationFilter: {
    // TODO(quacht): remove this exception once the color contrast issue is
    // solved.
    // http://crbug.com/748608
    'color-contrast': function(nodeResult) {
      return nodeResult.element.id == 'prompt';
    },
    'aria-valid-attr': function(nodeResult) {
      return nodeResult.element.hasAttribute('aria-active-attribute');
    },
  },
  rulesToSkip: ['skip-link']
};
/**
 * Test fixture for BASIC
 * @constructor
 * @extends {PolymerTest}
 */
function SettingsA11yBasic() {}

SettingsA11yBasic.prototype = {
  __proto__: SettingsAccessibilityTest.prototype,
};
SettingsAccessibilityTest.createTestF('SettingsA11yBasic', testDef);

GEN('#endif  // defined(NDEBUG)');
