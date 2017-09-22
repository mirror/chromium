// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Tests for interventions_internals.js
 */

/** @const {string} Path to source root. */
let ROOT_PATH = '../../../../';

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
      }

      /**
       * Setup testing map.
       * @param {Map} map The testing status map.
       */
      setTestingMap(map) {
        this.statuses_ = map;
      }

      /** @override **/
      getPreviewsEnabled() {
        this.methodCalled('getPreviewsEnabled');
        return Promise.resolve({
          statuses: this.statuses_,
        });
      }
    }

    window.setupFn = function() {
      return this.setupFnResolver.promise;
    }.bind(this);

    window.testPageHandler = new TestPageHandler();
  },
};

TEST_F('InterventionsInternalsUITest', 'DisplayCorrectStatuses', function() {
  let setupFnResolver = this.setupFnResolver;
  test('DisplayCorrectStatuses', function() {
    // Setup testPageHandler behavior.
    let testMap = new Map();
    testMap.set('params1', true);
    testMap.set('params2', false);
    testMap.set('params3', false);
    testMap.set('params4', true);
    testMap.set('params5', false);
    window.testPageHandler.setTestingMap(testMap);
    setupFnResolver.resolve();
    return setupFnResolver.promise
        .then(function() {
          return window.testPageHandler.whenCalled('getPreviewsEnabled');
        })
        .then(function() {
          testMap.forEach(function(value, key) {
            let expected = key + ': ' + (value ? 'Enabled' : 'Disabled');
            let actual = document.querySelector('#' + key).textContent;
            expectEquals(expected, actual);
          });
        });
  });
  mocha.run();
});
