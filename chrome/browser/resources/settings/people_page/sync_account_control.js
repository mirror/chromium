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

    availableAccounts_: Object,

    selectedAccount_: Object,

    promoLabel: String,

    promoSecondaryLabel: String,

    hasAvailableAccounts_: {
      type: Boolean,
      value: false,
      computed: 'computeHasAvailableAccounts_(availableAccounts_.*)'
    }
  },

  observers: ['onSignedInChanged_(syncStatus.signedIn, availableAccounts_.*)'],

  /** @private {?settings.SyncBrowserProxy} */
  syncBrowserProxy_: null,

  /** @override */
  attached: function() {
    this.syncBrowserProxy_ = settings.SyncBrowserProxyImpl.getInstance();
    this.syncBrowserProxy_.getSyncStatus().then(
        this.handleSyncStatus_.bind(this));
    this.addWebUIListener(
        'sync-status-changed', this.handleSyncStatus_.bind(this));

    // TODO(scottchen): use sync browser proxy
    // TODO(scottchen): needs to have webui-listener to update the list when
    // more accounts are available
    cr.sendWithPromise('SyncSetupGetAvailableAccounts').then(accounts => {
      this.availableAccounts_ = accounts;
    });
  },

  /**
   * Handler for when the sync state is pushed from the browser.
   * @param {?settings.SyncStatus} syncStatus
   * @private
   */
  handleSyncStatus_: function(syncStatus) {
    this.syncStatus = syncStatus;
  },

  computeHasAvailableAccounts_: function() {
    return this.availableAccounts_ && this.availableAccounts_.length > 0;
  },

  /** @private */
  onSigninTap_: function() {
    this.syncBrowserProxy_.startSignIn();
    /** @type {!CrActionMenuElement} */ (this.$$('#menu')).close();
  },

  onSyncButtonTap_: function() {
    chrome.send(
        'SyncSetupStartSyncingWithEmail', [this.selectedAccount_.email]);
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
    this.selectedAccount_ = e.model.item;
    /** @type {!CrActionMenuElement} */ (this.$$('#menu')).close();
  },

  /** @private */
  onSignedInChanged_: function() {
    if (this.syncStatus.signedIn) {
      this.selectedAccount_ = {
        fullName: this.syncStatus.signedInFullName,
        pictureUrl: this.syncStatus.signedInPictureUrl,
        email: this.syncStatus.signedInUsername,
      };
    } else {
      this.selectedAccount_ =
          this.availableAccounts_ ? this.availableAccounts_[0] : null;
    }
  }
});