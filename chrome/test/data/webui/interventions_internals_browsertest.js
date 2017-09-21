// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Tests for interventions_internals.js
 */

/** @const {string} Path to source root. */
var ROOT_PATH = '../../../../';

/**
 * Test fixture for InterventionsInternals WebUI testing.
 * @constructor
 * @extends testing.Test
 */
function InterventionsInternalsUITest() {
  this.setupFnResolver = new PromiseResolver();
}

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
    ROOT_PATH + 'ui/webui/resources/js/promise_resolver.js',
    ROOT_PATH + 'ui/webui/resources/js/util.js',
    ROOT_PATH + 'chrome/test/data/webui/test_browser_proxy.js',
  ],

  preLoad: function() {
    /**
     * A stub class for the Mojo PageHandler.
     */
    class TestPageHandler extends TestBrowserProxy {
      constructor() {
        super(['finishJob']);
        this.previewsEnabled_ = false;
      }

      /**
       * Change the behavior of the PageHandler for testing purposes.
       * @param {boolean} The new status for previews.
       */
      setPreviewsEnabled(enabled) {
        this.previewsEnabled_ = enabled;
      }

      /** @override */
      getPreviewsEnabled() {
        return Promise.resolve({
          enabled: this.previewsEnabled_,
        });
      }

      /**
       * Mark the job is finished.
       */
      finishJob() {
        this.methodCalled('finishJob');
      }
    }

    window.setupFn = function() {
      return this.setupFnResolver.promise;
    }.bind(this);

    window.testPageHandler = new TestPageHandler();
  },
};

TEST_F('InterventionsInternalsUITest', 'PreviewStatusEnabledTest', function() {
  let setupFnResolver = this.setupFnResolver;

  test('EnabledStatusTest', function() {
    // setup testPageHandler behavior
    window.testPageHandler.setPreviewsEnabled(true);
    setupFnResolver.resolve();

    return window.testPageHandler.whenCalled('finishJob')
        .then(function() {
          let message = document.querySelector('#log-message > p');
          expectEquals(message.textContent, 'Previews enabled: Enabled');
        });
  });

  mocha.run();
});

TEST_F('InterventionsInternalsUITest', 'PreviewStatusDisabledTest', function() {
  let setupFnResolver = this.setupFnResolver;

  test('DisabledStatusTest', function() {
    // setup testPageHandler behavior
    window.testPageHandler.setPreviewsEnabled(false);
    setupFnResolver.resolve();

    return window.testPageHandler.whenCalled('finishJob')
        .then(function() {
          let message = document.querySelector('#log-message > p');
          expectEquals(message.textContent, 'Previews enabled: Disabled');
        });
  });

  mocha.run();
});
