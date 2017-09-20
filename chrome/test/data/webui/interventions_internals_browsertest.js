// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Tests for interventions_internals.js
 */

/** @const {string} Path to source root. */
var ROOT_PATH = '../../../../';

/**
 * A stub class for the Mojo PageHandler.
 */
class TestPageHandler {
  constructor() {
    this.previewsEnabled_ = false;
  }

  /**
   * Change the behavior of the PageHandler for testing purposes.
   * @param {boolean} The new status for previews.
   */
  setPreviewsEnabled(enabled) {
    this.previewsEnabled_ = enabled;
  }

  /** @override **/
  getPreviewsEnabled() {
    return Promise.resolve({
      enabled: this.previewsEnabled_,
    });
  }
}

/**
 * Test fixture for InterventionsInternals WebUI testing.
 * @constructor
 * @extends testing.Test
 */
function InterventionsInternalsUITest() {}

InterventionsInternalsUITest.prototype = {
  __proto__: testing.Test.prototype,

  /**
   * Browse to the options page and call preLoad().
   * @override
   */
  browsePreload: 'chrome://interventions-internals',

  /** @override */
  isAsync: true,

  extraLibraries: [
    ROOT_PATH + 'third_party/mocha/mocha.js',
    ROOT_PATH + 'chrome/test/data/webui/mocha_adapter.js',
  ],

  preLoad: function() {
    window.testingMode = true;
    window.testPageHandler = new TestPageHandler();
  },
};

TEST_F('InterventionsInternalsUITest', 'PreviewStatusEnabledTest', function() {

  test('EnabledStatusTest', function() {
    // setup testPageHandler behavior
    window.testPageHandler.setPreviewsEnabled(true);

    return interventions_internals.getPreviewsEnabled().then(function() {
      let message = document.querySelector('#log-message > p');
      expectEquals(message.textContent, 'Previews enabled: Enabled');
    });
  });

  mocha.run();
});

TEST_F('InterventionsInternalsUITest', 'PreviewStatusDisabledTest', function() {
  test('DisabledStatusTest', function() {
    // setup testPageHandler behavior
    window.testPageHandler.setPreviewsEnabled(false);

    return interventions_internals.getPreviewsEnabled().then(function() {
      let message = document.querySelector('#log-message > p');
      expectEquals(message.textContent, 'Previews enabled: Disabled');
    });
  });

  mocha.run();
});
