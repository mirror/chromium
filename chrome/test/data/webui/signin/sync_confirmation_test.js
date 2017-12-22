// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('signin_sync_confirmation', function() {

  suite('SigninSyncConfirmationTest', function() {
    setup(function() {
      PolymerTest.clearBody();
      const app = document.createElement('sync-confirmation-app');
      document.body.append(app);
    });

    // This test needs no assertions - it just tests that no DCHECKS are
    // thrown during initialization of the UI.
    test('LoadPage', function() {});
  });
});
