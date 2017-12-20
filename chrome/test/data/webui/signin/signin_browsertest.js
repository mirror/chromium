// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Runs the Polymer Settings tests. */

/** @const {string} Path to source root. */
var ROOT_PATH = '../../../../../';

// Polymer BrowserTest fixture.
GEN_INCLUDE(
    [ROOT_PATH + 'chrome/test/data/webui/polymer_browser_test_base.js']);

/**
 * Test fixture for Polymer Sign-in elements.
 * @constructor
 * @extends {PolymerTest}
 */
function SigninBrowserTest() {}

SigninBrowserTest.prototype = {
  __proto__: PolymerTest.prototype,

  /** @override */
  get browsePreload() {
    throw 'this is abstract and should be overridden by subclasses';
  },

  setUp: function() {
    PolymerTest.prototype.setUp.call(this);
  },
};

// Have to include command_line.h manually due to GEN calls below.
GEN('#include "base/command_line.h"');

/**
 * Test fixture for
 * chrome/browser/resources/signin/dice_sync_confirmation/sync_confirmation.html.
 * @constructor
 * @extends {SigninBrowserTest}
 */
function SigninSyncConfirmationTest() {}

SigninSyncConfirmationTest.prototype = {
  __proto__: SigninBrowserTest.prototype,

  /** @override */
  browsePreload: 'chrome://sync-confirmation',

  /** @override */
  extraLibraries: SigninBrowserTest.prototype.extraLibraries.concat([
    'sync_confirmation_test.js',
  ]),
};

TEST_F('SigninSyncConfirmationTest', 'All', function() {
  mocha.run();
});
