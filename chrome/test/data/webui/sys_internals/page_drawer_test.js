// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var PageTest = PageTest || {};

PageTest.Drawer = function() {
  suite('Page drawer integration test', function() {
    suiteSetup('Wait for the page initialize.', function(done) {
      setTimeout(done, 1000);
    });

    function hasClass(element, cls) {
      return element.className.split(' ').indexOf(cls) != -1;
    }

    function isOpen() {
      return !($('sys-internals-drawer').hasAttribute('hidden')) &&
          !(hasClass($('sys-internals-drawer'), 'hidden')) &&
          !(hasClass($('sys-internals-drawer-menu'), 'hidden'));
    }

    function isClose() {
      return $('sys-internals-drawer').hasAttribute('hidden') &&
          hasClass($('sys-internals-drawer'), 'hidden') &&
          hasClass($('sys-internals-drawer-menu'), 'hidden');
    }

    function openAndCloseTest(openFun, closeFun, done) {
      if (!isClose()) {
        done(new Error('Drawer is not closed.'));
        return;
      }
      openFun();
      setTimeout(function() {
        if (!isOpen()) {
          done(new Error('Drawer is not opened.'));
          return;
        }
        closeFun();
        setTimeout(function() {
          if (!isClose()) {
            done(new Error('Drawer is not closed.'));
            return;
          }
          done();
        }, 300);
      }, 300);
    }

    test('open and close', function(done) {
      this.timeout(2000);
      openAndCloseTest(openDrawer, closeDrawer, done);
    });

    function open() {
      MockInteractions.tap($('sys-internals-nav-menu-btn'));
    }

    function close1() {
      MockInteractions.tap($('sys-internals-drawer'));
    }

    function close2() {
      const infoBtn =
          document.getElementsByClassName('sys-internals-drawer-item')[0];
      MockInteractions.tap(infoBtn);
    }

    test('Tap to open and close1', function(done) {
      this.timeout(2000);
      openAndCloseTest(open, close1, done);
    });

    test('Tap to open and close2', function(done) {
      this.timeout(2000);
      openAndCloseTest(open, close2, done);
    });
  });

  mocha.run();
};
