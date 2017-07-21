// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 * @implements {settings.SyncBrowserProxy}
 * @extends {TestBrowserProxy}
 */
var TestSyncBrowserProxy = function() {
  TestBrowserProxy.call(this, [
    'getSyncStatus',
    'signOut',
  ]);
};

TestSyncBrowserProxy.prototype = {
  __proto__: TestBrowserProxy.prototype,

  /** @override */
  getSyncStatus: function() {
    this.methodCalled('getSyncStatus');
    return Promise.resolve({signedIn: true, signedInUsername: 'fakeUsername'});
  },

  /** @override */
  signOut: function(deleteProfile) {
    this.methodCalled('signOut', deleteProfile);
  },
};