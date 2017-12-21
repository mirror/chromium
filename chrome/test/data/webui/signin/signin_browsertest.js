// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Runs the Polymer Settings tests. */

/** @const {string} Path to source root. */
var ROOT_PATH = '../../../../../';

// Polymer BrowserTest fixture.
GEN_INCLUDE(
    [ROOT_PATH + 'chrome/test/data/webui/polymer_browser_test_base.js']);
GEN('#include "base/command_line.h"');
GEN('#include "chrome/test/data/webui/signin_browsertest.h"');

/**
 * Test fixture for
 * chrome/browser/resources/signin/dice_sync_confirmation/sync_confirmation.html.
 * @constructor
 * @extends {PolymerTest}
 */
function SigninSyncConfirmationTest() {}

SigninSyncConfirmationTest.prototype = {
  __proto__: PolymerTest.prototype,

  typedefCppFixture: 'SigninBrowserTest',

  testGenPreamble: function() {
    GEN('  EnableDice();');
  },

  /** @override */
  browsePreload: 'chrome://sync-confirmation/sync_confirmation_app.html',

  /** @override */
  extraLibraries: PolymerTest.getLibraries(ROOT_PATH).concat([
    'sync_confirmation_test.js',
  ]),
};

TEST_F('SigninSyncConfirmationTest', 'DialogWithDice', function() {
  mocha.run();
});
