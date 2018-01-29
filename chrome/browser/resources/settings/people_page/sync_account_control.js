// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/**
 * @fileoverview
 * 'settings-sync-account-section' is the settings page containing sign-in
 * settings.
 */
Polymer({
  is: 'settings-sync-account-control',
  behaviors: [WebUIListenerBehavior],
  properties: {
    /**
     * The current sync status, supplied by SyncBrowserProxy.
     * @type {?settings.SyncStatus}
     */
    syncStatus: Object,

    storedAccounts_: Object,

    shownAccount_: Object,

    promoLabel: String,

    promoSecondaryLabel: String,

    shouldShowAvatarRow_: {
      type: Boolean,
      value: false,
      computed: 'computeShouldShowAvatarRow_(storedAccounts_.*)'
    }
  },

  observers: [
    'onShownAccountShouldChange_(storedAccounts_.*, syncStatus.*)',
  ],

  /** @private {?settings.SyncBrowserProxy} */
  syncBrowserProxy_: null,

  /** @override */
  attached: function() {
    this.syncBrowserProxy_ = settings.SyncBrowserProxyImpl.getInstance();
    this.syncBrowserProxy_.getSyncStatus().then(
        this.handleSyncStatus_.bind(this));
    this.addWebUIListener(
        'sync-status-changed', this.handleSyncStatus_.bind(this));
    this.addWebUIListener(
        'stored-accounts-updated', this.handleStoredAccounts_.bind(this));

    // TODO(scottchen): use sync browser proxy
    // TODO(scottchen): needs to have webui-listener to update the list when
    // more accounts are available
    cr.sendWithPromise('SyncSetupGetStoredAccounts')
        .then(this.handleStoredAccounts_.bind(this));
  },

  /** @private */
  getSubstituteLabel_: function(label, name) {
    return loadTimeData.substituteString(label, name);
  },

  /** @private */
  handleStoredAccounts_: function(accounts) {
    this.storedAccounts_ = accounts;
  },

  /**
   * Handler for when the sync state is pushed from the browser.
   * @param {?settings.SyncStatus} syncStatus
   * @private
   */
  handleSyncStatus_: function(syncStatus) {
    this.syncStatus = syncStatus;
  },

  /** @private */
  computeShouldShowAvatarRow_: function() {
    return this.syncStatus.signedIn ||
        (this.storedAccounts_ && this.storedAccounts_.length > 0);
  },

  /** @private */
  onSigninTap_: function() {
    this.syncBrowserProxy_.startSignIn();
    /** @type {!CrActionMenuElement} */ (this.$$('#menu')).close();
  },

  onSyncButtonTap_: function() {
    chrome.send('SyncSetupStartSyncingWithEmail', [this.shownAccount_.email]);
  },

  /** @private */
  onDisconnectTap_: function() {
    /* This will route to people_page's disconnect dialog. */
    settings.navigateTo(settings.routes.SIGN_OUT);
  },

  /** @private */
  onMenuButtonTap_: function() {
    const actionMenu =
        /** @type {!CrActionMenuElement} */ (this.$$('#menu'));
    actionMenu.showAt(assert(this.$$('#dots')));
  },

  /**
   * @param {!Event}
   * @private
   */
  onAccountTap_: function(e) {
    this.shownAccount_ = e.model.item;
    /** @type {!CrActionMenuElement} */ (this.$$('#menu')).close();
  },

  /** @private */
  onShownAccountShouldChange_: function() {
    if (this.syncStatus.signedIn) {
      for (let i = 0; i < this.storedAccounts_.length; i++) {
        if (this.storedAccounts_[i].email == this.syncStatus.signedInUsername) {
          this.shownAccount_ = this.storedAccounts_[i];
          return;
        }
      }
    } else {
      this.shownAccount_ =
          this.storedAccounts_ ? this.storedAccounts_[0] : null;
    }
  }
});