// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @typedef {{
 *   tetherNostDeviceName: string,
 *   batteryPercentage: number,
 *   connectionStrength: number,
 *   isTetherHostCurrentlyOnWifi: boolean
 * }}
 */
var TetherConnectionData;

Polymer({
  is: 'tether-connection-dialog',

  behaviors: [I18nBehavior],

  properties: {
    /**
     * The current properties for the network matching |guid|.
     * @type {!CrOnc.NetworkProperties|undefined}
     */
    networkProperties: {
      type: Object,
    },
  },

  open: function() {
    var dialog = this.getDialog_();
    if (!dialog.open)
      this.getDialog_().showModal();

    this.$.connectButton.focus();
  },

  close: function() {
    var dialog = this.getDialog_();
    if (dialog.open)
      dialog.close();
  },

  /**
   * @return {!CrDialogElement}
   * @private
   */
  getDialog_: function() {
    return /** @type {!CrDialogElement} */ (this.$.dialog);
  },

  /** @private */
  onNotNowTap_: function() {
    this.getDialog_().cancel();
  },

  /**
   * Fires the 'connect-tap' event.
   * @private
   */
  onConnectTap_: function() {
    this.fire('tether-connect');
  },

  /**
   * @param {!CrOnc.NetworkProperties} networkProperties The network
   *     properties.
   * @return {boolean}
   * @private
   */
  shouldShowDisconnectFromWifi_: function(networkProperties) {
    // TODO(khorimoto): Pipe through a new network property which describes
    // whether the tether host is currently connected to a Wi-Fi network. Return
    // whether it is here.
    return true;
  },

  /**
   * @param {!CrOnc.NetworkProperties} networkProperties The network properties.
   * @return {string} The battery percentage integer value converted to a
   *     string. Note that this will not return a string with a "%" suffix.
   * @private
   */
  getBatteryPercentageAsString_: function(networkProperties) {
    var percentage = this.get('Tether.BatteryPercentage', networkProperties);
    if (percentage === undefined)
      return '';
    return percentage.toString();
  },

  /**
   * Retrieves a confirmation image that corresponds to signal
   * strength of the tether host.  Custom icons are used here instead of a
   * <cr-network-icon> because this dialog uses a special color scheme.
   *
   * @param {!CrOnc.NetworkProperties} networkProperties The network properties.
   * @return {string} The name of the icon to be used to represent the network's
   * signal strength.
   */
  getSignalStrength_: function(networkProperties) {
    var percentage = this.get('Tether.SignalStrength', networkProperties);
    var signalStrength;
    if (percentage >= 0 && percentage <= 24) {
      signalStrength = '0';
    } else if (percentage >= 25 && percentage <= 49) {
      signalStrength = '1';
    } else if (percentage >= 50 && percentage <= 74) {
      signalStrength = '2';
    } else if (percentage >= 75 && percentage <= 99) {
      signalStrength = '3';
    } else {
      signalStrength = '4';
    }
    return 'settings:signal-cellular-' + signalStrength + '-bar';
  },

  /**
   * @param {!CrOnc.NetworkProperties} networkProperties The network properties.
   * @return {string}
   * @private
   */
  getDeviceName_: function(networkProperties) {
    return CrOnc.getNetworkName(networkProperties);
  },

  /**
   * @param {!CrOnc.NetworkProperties} networkProperties The network properties.
   * @return {string}
   * @private
   */
  getBatteryPercentageString_: function(networkProperties) {
    return this.i18n(
        'tetherConnectionBatteryPercentage',
        this.getBatteryPercentageAsString_(networkProperties));
  },

  /**
   * @param {!CrOnc.NetworkProperties} networkProperties The network properties.
   * @return {string}
   * @private
   */
  getExplanation_: function(networkProperties) {
    return this.i18n(
        'tetherConnectionExplanation', CrOnc.getNetworkName(networkProperties));
  },

  /**
   * @param {!CrOnc.NetworkProperties} networkProperties The network properties.
   * @return {string}
   * @private
   */
  getDescriptionTitle_: function(networkProperties) {
    return this.i18n(
        'tetherConnectionDescriptionTitle',
        CrOnc.getNetworkName(networkProperties));
  },

  /**
   * @param {!CrOnc.NetworkProperties} networkProperties The network properties.
   * @return {string}
   * @private
   */
  getBatteryDescription_: function(networkProperties) {
    return this.i18n(
        'tetherConnectionDescriptionBattery',
        this.getBatteryPercentageAsString_(networkProperties));
  },
});
