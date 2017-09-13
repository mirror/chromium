// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var PageTest = PageTest || {};

PageTest.Drawer = function() {
  suite('Page drawer integration test', function() {
    suiteSetup('Wait for the page initialize.', function(done) {
      SysInternals.waitSysInternalsInitialized.promise.then(done);
    });

    function checkOpen() {
      assertFalse($('sys-internals-drawer').hasAttribute('hidden'));
      assertFalse($('sys-internals-drawer').classList.contains('hidden'));
      assertFalse($('sys-internals-drawer-menu').classList.contains('hidden'));
    }

    function checkClose() {
      assertTrue($('sys-internals-drawer').hasAttribute('hidden'));
      assertTrue($('sys-internals-drawer').classList.contains('hidden'));
      assertTrue($('sys-internals-drawer-menu').classList.contains('hidden'));
    }

    function operate(action, checker) {
      SysInternals.waitDrawerActionCompleted = new PromiseResolver();
      action();
      return SysInternals.waitDrawerActionCompleted.promise.then(function() {
        return new Promise(function(resolve, reject) {
          checker();
          resolve();
        });
      });
    }

    test('open and close by Sysinternals function', function(done) {
      operate(openDrawer, checkOpen)
          .then(function() {
            return operate(closeDrawer, checkClose);
          })
          .then(function() {
            return operate(openDrawer, checkOpen);
          })
          .then(function() {
            return operate(closeDrawer, checkClose);
          })
          .then(done, done);
    });

    function openByButton() {
      MockInteractions.tap($('sys-internals-nav-menu-btn'));
    }

    function closeByClickBackground() {
      MockInteractions.tap($('sys-internals-drawer'));
    }

    function closeByClickInfoPageButton() {
      const infoPageBtn =
          document.getElementsByClassName('sys-internals-drawer-item')[0];
      MockInteractions.tap(infoPageBtn);
    }

    test('Tap to open and close', function(done) {
      operate(openByButton, checkOpen)
          .then(function() {
            return operate(closeByClickBackground, checkClose);
          })
          .then(function() {
            return operate(openByButton, checkOpen);
          })
          .then(function() {
            return operate(closeByClickInfoPageButton, checkClose);
          })
          .then(done, done);
    });
  });

  mocha.run();
};
