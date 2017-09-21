// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('settings_rtl_tests', function() {
  /** @implements {settings.DirectionDelegate} */
  function TestDirectionDelegate(isRtl) {
    /** @override */
    this.isRtl = function() { return isRtl; };
  }

  suite('settings drawer panel RTL tests', function() {
    setup(function() {
      PolymerTest.clearBody();
    });

    test('test drawer respects loadTimeData LTR', function() {
      settings.setDirectionDelegateForTesting(
          new TestDirectionDelegate(false /* isRtl */));
      var ui = document.createElement('settings-ui');
      document.body.appendChild(ui);
      assertEquals('left', ui.$.drawer.align);
    });

    test('test drawer respects loadTimeData RTL', function() {
      settings.setDirectionDelegateForTesting(
          new TestDirectionDelegate(true /* isRtl */));
      var ui = document.createElement('settings-ui');
      document.body.appendChild(ui);
      assertEquals('right', ui.$.drawer.align);
    });
  });
});
