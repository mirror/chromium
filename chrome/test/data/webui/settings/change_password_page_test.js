// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @implements {settings.ChangePasswordBrowserProxy} */
class TestChangePasswordBrowserProxy extends TestBrowserProxy {
  constructor() {
    super([
      'initializeChangePasswordHandler',
      'onChangePasswordPageShown',
      'changePassword',
    ]);
  }

  /** @override */
  initializeChangePasswordHandler() {
    this.methodCalled('initializeChangePasswordHandler');
  }

  /** @override */
  onChangePasswordPageShown() {
    this.methodCalled('onChangePasswordPageShown');
  }

  /** @override */
  changePassword() {
    this.methodCalled('changePassword');
  }
}

var changePasswordPage = null;

/** @type {?TestChangePasswordBrowserProxy} */
var ChangePasswordBrowserProxy = null;

suite('ChangePasswordHandler', function() {
  setup(function() {
    ChangePasswordBrowserProxy = new TestChangePasswordBrowserProxy();
    settings.ChangePasswordBrowserProxyImpl.instance_ =
        ChangePasswordBrowserProxy;

    PolymerTest.clearBody();

    changePasswordPage =
        document.createElement('settings-change-password-page');
    document.body.appendChild(changePasswordPage);
  });

  teardown(function() {
    changePasswordPage.remove();
  });

  test('changePasswordButtonPressed', function() {
    var actionButton = changePasswordPage.$$('#changePassword');
    assertTrue(!!actionButton);
    MockInteractions.tap(actionButton);
    return ChangePasswordBrowserProxy.whenCalled('changePassword');
  });
});
