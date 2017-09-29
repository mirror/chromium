// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'internet-config-dialog' is used to configure a new or existing network
 * outside of settings (e.g. from the login screen or when configuring a
 * new network from the system tray).
 */
Polymer({
  is: 'internet-config-dialog',

  behaviors: [I18nBehavior],

  properties: {
    /**
     * Interface for networkingPrivate calls.
     * @type {NetworkingPrivate}
     */
    networkingPrivate: {
      type: Object,
      value: chrome.networkingPrivate,
    },

    /** The network GUID to configure for (empty if for a new network). */
    guid_: String,

    /**
     * The type of network is being configured.
     * @private {!chrome.networkingPrivate.NetworkType}
     */
    type_: String,

    /**
     * The network of the network being configured (set by network-config).
     * @private
     */
    name_: {
      type: String,
      value: '',
    },

    /** @private */
    enableConnect_: String,

    /** @private */
    enableSave_: String,

    /**
     * The current properties if an existing network being configured, or
     * a minimal subset for a new network.
     * @type {!CrOnc.NetworkProperties|undefined}
     */
    networkProperties_: Object,
  },

  /** @override */
  attached: function() {
    var dialogArgs = chrome.getVariableValue('dialogArguments');
    assert(dialogArgs);
    var args = JSON.parse(dialogArgs);
    this.type_ = args.type;
    assert(this.type_);
    this.guid_ = args.guid;

    this.networkProperties_ = {
      GUID: this.guid_,
      Name: this.name_,
      Type: this.type_,
    };

    this.$.networkConfig.init();
  },

  /** @private */
  close_: function() {
    chrome.send('dialogClose');
  },

  /**
   * @return {string}
   * @private
   */
  getTitle_: function() {
    return this.name_ || this.i18n('OncType' + this.type_);
  },

  /** @private */
  onCancelTap_: function() {
    this.close_();
  },

  /** @private */
  onSaveTap_: function() {
    this.$.networkConfig.saveOrConnect();
  },

  /** @private */
  onConnectTap_: function() {
    this.$.networkConfig.saveOrConnect();
  },
});
