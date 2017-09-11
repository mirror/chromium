// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

suite('cr-toast', function() {
  var toast;

  setup(function() {
    PolymerTest.clearBody();
    toast = document.createElement('cr-toast');
    document.body.appendChild(toast);
  });

  test('simple show/hide', function() {
    assertFalse(toast.open_);

    toast.show();
    assertTrue(toast.open_);

    toast.hide();
    assertFalse(toast.open_);
  });

  test('auto hide', function() {
    var timerProxy = new TestTimerProxy();
    timerProxy.immediatelyResolveTimeouts = false;
    toast.setTimerProxyForTesting(timerProxy);

    toast.duration = 100;

    toast.show();
    assertEquals(0, toast.hideTimeoutId_);
    assertTrue(toast.open_);

    timerProxy.runTimeoutFn(0);
    assertEquals(null, toast.hideTimeoutId_);
    assertFalse(toast.open_);

    // Check that multiple shows reset the timeout.
    toast.show();
    assertEquals(1, toast.hideTimeoutId_);
    assertTrue(toast.open_);

    toast.show();
    assertFalse(timerProxy.hasTimeout(1));
    assertEquals(2, toast.hideTimeoutId_);
    assertTrue(toast.open_);
  });
});
