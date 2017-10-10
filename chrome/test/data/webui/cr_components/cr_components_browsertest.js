// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Tests for shared Polymer components. */

/** @const {string} Path to source root. */
var ROOT_PATH = '../../../../../';

// Polymer BrowserTest fixture.
GEN_INCLUDE(
    [ROOT_PATH + 'chrome/test/data/webui/polymer_browser_test_base.js']);

/**
 * Test fixture for shared Polymer components.
 * @constructor
 * @extends {PolymerTest}
 */
function CrComponentsBrowserTest() {}

CrComponentsBrowserTest.prototype = {
  __proto__: PolymerTest.prototype,

  /** @override */
  extraLibraries: PolymerTest.getLibraries(ROOT_PATH),

  /** @override */
  get browsePreload() {
    throw 'this is abstract and should be overriden by subclasses';
  },

  /** @override */
  runAccessibilityChecks: true,

  /** @override */
  setUp: function() {
    PolymerTest.prototype.setUp.call(this);
    // We aren't loading the main document.
    this.accessibilityAuditConfig.ignoreSelectors('humanLangMissing', 'html');
  },
};

GEN('#if defined(OS_CHROMEOS)');

/**
 * @constructor
 * @extends {CrComponentsBrowserTest}
 */
function CrComponentsNetworkConfigTest() {}

CrComponentsNetworkConfigTest.prototype = {
  __proto__: CrComponentsBrowserTest.prototype,

  /** @override */
  browsePreload:
      'chrome://resources/cr_components/chromeos/network/network_config.html',

  /** @override */
  extraLibraries: CrComponentsBrowserTest.prototype.extraLibraries.concat([
    '../../../../../third_party/closure_compiler/externs/networking_private.js',
    '../fake_chrome_event.js',
    '../chromeos/fake_networking_private.js',
    '../chromeos/cr_onc_strings.js',
    'network_config_test.js',
  ]),
};

TEST_F('CrComponentsNetworkConfigTest', 'All', function() {
  mocha.run();
});

GEN('#endif');
