// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for displaying and modifying a list of cellular
 * mobile networks.
 */
Polymer({
  is: 'network-choose-mobile',

  behaviors: [I18nBehavior],

  properties: {
    /**
     * The current set of properties for the network.
     * @type {!CrOnc.NetworkProperties|undefined}
     */
    networkProperties: {
      type: Object,
      observer: 'networkPropertiesChanged_',
    },

    /**
     * The networkingPrivate.FoundNetworkProperties.NetworkId of the selected
     * mobile network.
     * @private
     */
    selectedMobileNetworkId_: {
      type: String,
      value: '',
    },

    /**
     * Selectable list of FoundNetwork dictionaries for the UI.
     * @private {!Array<!chrome.networkingPrivate.FoundNetworkProperties>}
     */
    mobileNetworkList_: {
      type: Array,
      value: function() {
        return [];
      }
    },
  },

  /**
   * Polymer networkProperties changed method.
   */
  networkPropertiesChanged_: function() {
    if (!this.networkProperties || !this.networkProperties.Cellular)
      return;

    this.mobileNetworkList_ =
        this.networkProperties.Cellular.FoundNetworks || [];
    if (this.mobileNetworkList_.length < 1)
      return;
    var selected = this.mobileNetworkList_.find(function(mobileNetwork) {
      return mobileNetwork.Status == 'current';
    });
    if (!selected)
      selected = this.mobileNetworkList_[0];
    this.selectedMobileNetworkId_ = selected.NetworkId;
  },

  /**
   * @param {!chrome.networkingPrivate.FoundNetworkProperties} network
   * @return {boolean}
   * @private
   */
  getIsDisabled_: function(network) {
    return network.Status != 'available' && network.Status != 'current';
  },

  /**
   * @param {!chrome.networkingPrivate.FoundNetworkProperties} network
   * @return {string}
   * @private
   */
  getName_: function(network) {
    return network.LongName || network.ShortName || network.NetworkId;
  },

  /**
   * @param {!Event} event
   * @private
   */
  onChange_: function(event) {
    var target = /** @type {!HTMLSelectElement} */ (event.target);
    this.fire('change', target.value);
  },
});
