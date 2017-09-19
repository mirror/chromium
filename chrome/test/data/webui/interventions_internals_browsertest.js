// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Tests for interventions_internals.js
 */

/** @const {string} Path to source root. */
var ROOT_PATH = '../../../../';

/**
 * Test fixture for BluetoothInternals WebUI testing.
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

  isAsync: true,

  preLoad: function() {
    window.testPageHandler = {};
    window.testPageHandler.getPreviewsEnabled = null;
  },
};

TEST_F('InterventionsInternalsUITest', 'PreviewsMessageIsEnabled', function() {
  // setup testPageHandler behavior
  window.testPageHandler.getPreviewsEnabled = function() {
    return Promise.resolve({
      enabled: true,
    });
  };

  interventions_internals.getPreviewsEnabled().then(function() {
    let message = document.querySelector('#log-message > p');
    expectEquals(message.textContent, 'Previews enabled: Enabled');
    testDone();
  });
});

TEST_F('InterventionsInternalsUITest', 'PreviewsMessageIsDisabled', function() {
  // setup testPageHandler behavior
  window.testPageHandler.getPreviewsEnabled = function() {
    return Promise.resolve({
      enabled: false,
    });
  };

  interventions_internals.getPreviewsEnabled().then(function() {
    let message = document.querySelector('#log-message > p');
    expectEquals(message.textContent, 'Previews enabled: Disabled');
    testDone();
  });
});
