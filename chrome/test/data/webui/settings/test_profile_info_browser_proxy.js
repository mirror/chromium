// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @constructor
 * @implements {settings.ProfileInfoBrowserProxy}
 * @extends {TestBrowserProxy}
 */
var TestProfileInfoBrowserProxy = function() {
  TestBrowserProxy.call(this, [
    'getProfileInfo',
    'getProfileStatsCount',
    'getProfileManagesSupervisedUsers',
  ]);

  this.fakeProfileInfo = {
    name: 'fakeName',
    iconUrl: 'http://fake-icon-url.com/',
  };
};

TestProfileInfoBrowserProxy.prototype = {
  __proto__: TestBrowserProxy.prototype,

  /** @override */
  getProfileInfo: function() {
    this.methodCalled('getProfileInfo');
    return Promise.resolve(this.fakeProfileInfo);
  },

  /** @override */
  getProfileStatsCount: function() {
    this.methodCalled('getProfileStatsCount');
  },

  /** @override */
  getProfileManagesSupervisedUsers: function() {
    this.methodCalled('getProfileManagesSupervisedUsers');
    return Promise.resolve(false);
  }
};
