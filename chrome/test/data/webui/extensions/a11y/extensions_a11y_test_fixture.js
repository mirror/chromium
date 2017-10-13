// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Runs the Polymer Accessibility Extensions tests. */

/** @const {string} Path to root from chrome/test/data/webui/extensions/a11y. */
var ROOT_PATH = '../../../../../../';

// Polymer BrowserTest fixture and aXe-core accessibility audit.
GEN_INCLUDE([
  ROOT_PATH + 'chrome/test/data/webui/a11y/accessibility_test.js',
  ROOT_PATH + 'chrome/test/data/webui/polymer_browser_test_base.js',
]);

/**
 * Test fixture for Accessibility of Chrome Extensions.
 * @constructor
 * @extends {PolymerTest}
 */
var ExtensionsA11yTestFixture = class extends PolymerTest {
  /** @override */
  get browsePreload() {
    return 'chrome://extensions/';
  }

  /** @override */
  get commandLineSwitches() {
    return [{
      switchName: 'enable-features',
      switchValue: 'MaterialDesignExtensions',
    }];
  }

  // Include files that define the mocha tests.
  get extraLibraries() {
    return PolymerTest.getLibraries(ROOT_PATH);
  }

  // Default accessibility audit options. Specify in test definition to use.
  static get axeOptions() {
    return {
      'rules': {
        // Disable 'skip-link' check since there are few tab stops before the
        // main content.
        'skip-link': {enabled: false},
        // TODO(crbug.com/761461): enable after addressing flaky tests.
        'color-contrast': {enabled: false},
      }
    };
  }
};
