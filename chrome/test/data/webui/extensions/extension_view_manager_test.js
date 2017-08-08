// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Suite of tests for extensions-detail-view. */
cr.define('extension_view_manager_tests', function() {
  /** @enum {string} */
  var TestNames = {
    Visibility: 'visibility',
    EventFiring: 'event firing',
  };

  var viewManager;
  var views;

  suite('ExtensionViewManagerTest', function() {

    // Initialize an extension item before each test.
    setup(function() {
      PolymerTest.clearBody();

      document.body.innerHTML = `
        <extensions-view-manager id="viewManager">
          <div slot="view" id="viewOne">view 1</div>
          <div slot="view" id="viewTwo">view 2</div>
          <div slot="view" id="viewThree">view 3</div>
        </extensions-view-manager>`;

      viewManager = document.body.querySelector('#viewManager');
    });

    test(assert(TestNames.Visibility), function() {
      function isViewVisible(id) {
        return extension_test_util.isVisible(viewManager, '#' + id, true);
      }

      expectFalse(isViewVisible('viewOne'));
      expectFalse(isViewVisible('viewTwo'));
      expectFalse(isViewVisible('viewThree'));

      return viewManager.switchView('viewOne')
          .then(() => {
            expectTrue(isViewVisible('viewOne'));
            expectFalse(isViewVisible('viewTwo'));
            expectFalse(isViewVisible('viewThree'));

            return viewManager.switchView('viewThree')
          })
          .then(() => {
            expectFalse(isViewVisible('viewOne'));
            expectFalse(isViewVisible('viewTwo'));
            expectTrue(isViewVisible('viewThree'));
          });
    });

    test(assert(TestNames.EventFiring), function() {
      var viewOne = viewManager.querySelector('#viewOne');
      var enterStart = false;
      var enterFinish = false;
      var exitStart = false;
      var exitFinish = false;

      viewOne.addEventListener('view-enter-start', function() {
        enterStart = true;
      });

      viewOne.addEventListener('view-enter-finish', function() {
        enterFinish = true;
      });

      viewOne.addEventListener('view-exit-start', function() {
        exitStart = true;
      });

      viewOne.addEventListener('view-exit-finish', function() {
        exitFinish = true;
      });

      // Setup the switch promise first.
      var enterPromise = viewManager.switchView('viewOne');
      // view-enter-start should fire synchronously.
      expectTrue(enterStart);
      // view-enter-finish should not fire yet.
      expectFalse(enterFinish);

      return enterPromise
          .then(() => {
            // view-enter-finish should fire after animation.
            expectTrue(enterFinish);

            enterPromise = viewManager.switchView('viewTwo');
            // view-exit-start should fire synchronously.
            expectTrue(exitStart);
            // view-exit-finish should not fire yet.
            expectFalse(exitFinish);

            return enterPromise;
          })
          .then(() => {
            // view-exit-finish should fire after animation.
            expectTrue(exitFinish);
          });
    });
  });

  return {
    TestNames: TestNames,
  };
});
