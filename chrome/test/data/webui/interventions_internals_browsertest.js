// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Tests for interventions_internals.js
 */

/** @const {string} Path to source root. */
let ROOT_PATH = '../../../../';
const AMP_REDIRECTION_PREVIEW = 'ampRedirectionPreview';
const CLIENT_LO_FI = 'clientLoFi';
const OFFLINE_PREVIEWS = 'offlinePreviews';

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
        super(['getPreviewsEnabled']);
        this.statuses_ = new Map();
        this.statuses_.set(AMP_REDIRECTION_PREVIEW, false);
        this.statuses_.set(CLIENT_LO_FI, false);
        this.statuses_.set(OFFLINE_PREVIEWS, false);
      }

      /**
       * Change the behavior of AMPRedirectionPreview in the PageHandler for
       * testing purposes.
       * @param {boolean} The new status for AMP_REDIRECTION_PREVIEW.
       */
      setAMPRedirectionPreview(enabled) {
        this.statuses_.set(AMP_REDIRECTION_PREVIEW, enabled);
      }

      /**
       * Change the behavior of ClientLoFi in the PageHandler for testing
       * purposes.
       * @param {boolean} The new status for CLIENT_LO_FI.
       */
      setClientLoFi(enabled) {
        this.statuses_.set(CLIENT_LO_FI, enabled);
      }

      /**
       * Change the behavior of OfflinePreviews in the PageHandler for testing
       * purposes.
       * @param {boolean} The new status for OFFLINE_PREVIEWS.
       */
      setOfflinePreviews(enabled) {
        this.statuses_.set(OFFLINE_PREVIEWS, enabled);
      }

      /** @override **/
      getPreviewsEnabled() {
        this.methodCalled('getPreviewsEnabled');
        return Promise.resolve({
          statuses: this.statuses_,
        });
      }

      /**
       * Mark the job is finished.
       */
      setupFnCompleted() {
        this.methodCalled('setupFnCompleted');
      }
    }

    window.setupFn = function() {
      return this.setupFnResolver.promise;
    }.bind(this);

    window.testPageHandler = new TestPageHandler();
  },
};

TEST_F(
    'InterventionsInternalsUITest', 'AMPRedirectionPreviewStatusEnabled',
    function() {
      let setupFnResolver = this.setupFnResolver;

      test('EnabledStatusTest', function() {
        // setup testPageHandler behavior
        window.testPageHandler.setAMPRedirectionPreview(true);
        setupFnResolver.resolve();

        return setupFnResolver.promise
            .then(function() {
              return window.testPageHandler.whenCalled('getPreviewsEnabled');
            })
            .then(function() {
              let message =
                  document.querySelector('#' + AMP_REDIRECTION_PREVIEW);
              expectEquals(
                  message.textContent, AMP_REDIRECTION_PREVIEW + ': Enabled');
            });
      });

      mocha.run();
    });

TEST_F(
    'InterventionsInternalsUITest', 'AMPRedirectionPreviewStatusDisabled',
    function() {
      let setupFnResolver = this.setupFnResolver;

      test('DisabledStatusTest', function() {
        // setup testPageHandler behavior
        window.testPageHandler.setAMPRedirectionPreview(false);
        setupFnResolver.resolve();

        return setupFnResolver.promise
            .then(function() {
              return window.testPageHandler.whenCalled('getPreviewsEnabled');
            })
            .then(function() {
              let message =
                  document.querySelector('#' + AMP_REDIRECTION_PREVIEW);
              expectEquals(
                  message.textContent, AMP_REDIRECTION_PREVIEW + ': Disabled');
            });
      });

      mocha.run();
    });

TEST_F('InterventionsInternalsUITest', 'ClientLoFiStatusEnabled', function() {
  let setupFnResolver = this.setupFnResolver;

  test('EnabledStatusTest', function() {
    // setup testPageHandler behavior
    window.testPageHandler.setClientLoFi(true);
    setupFnResolver.resolve();

    return setupFnResolver.promise
        .then(function() {
          return window.testPageHandler.whenCalled('getPreviewsEnabled');
        })
        .then(function() {
          let message = document.querySelector('#' + CLIENT_LO_FI);
          expectEquals(message.textContent, CLIENT_LO_FI + ': Enabled');
        });
  });

  mocha.run();
});

TEST_F('InterventionsInternalsUITest', 'ClientLoFiStatusDisabled', function() {
  let setupFnResolver = this.setupFnResolver;

  test('DisabledStatusTest', function() {
    // setup testPageHandler behavior
    window.testPageHandler.setClientLoFi(false);
    setupFnResolver.resolve();

    return setupFnResolver.promise
        .then(function() {
          return window.testPageHandler.whenCalled('getPreviewsEnabled');
        })
        .then(function() {
          let message = document.querySelector('#' + CLIENT_LO_FI);
          expectEquals(message.textContent, CLIENT_LO_FI + ': Disabled');
        });
  });

  mocha.run();
});

TEST_F(
    'InterventionsInternalsUITest', 'OfflinePreviewsStatusEnabled', function() {
      let setupFnResolver = this.setupFnResolver;

      test('EnabledStatusTest', function() {
        // setup testPageHandler behavior
        window.testPageHandler.setOfflinePreviews(true);
        setupFnResolver.resolve();

        return setupFnResolver.promise
            .then(function() {
              return window.testPageHandler.whenCalled('getPreviewsEnabled');
            })
            .then(function() {
              let message = document.querySelector('#' + OFFLINE_PREVIEWS);
              expectEquals(message.textContent, OFFLINE_PREVIEWS + ': Enabled');
            });
      });

      mocha.run();
    });

TEST_F(
    'InterventionsInternalsUITest', 'OfflinePreviewsStatusDisabled',
    function() {
      let setupFnResolver = this.setupFnResolver;

      test('DisabledStatusTest', function() {
        // setup testPageHandler behavior
        window.testPageHandler.setOfflinePreviews(false);
        setupFnResolver.resolve();

        return setupFnResolver.promise
            .then(function() {
              return window.testPageHandler.whenCalled('getPreviewsEnabled');
            })
            .then(function() {
              let message = document.querySelector('#' + OFFLINE_PREVIEWS);
              expectEquals(
                  message.textContent, OFFLINE_PREVIEWS + ': Disabled');
            });
      });

      mocha.run();
    });
