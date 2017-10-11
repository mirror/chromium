// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Fake implementation of chrome.networkingPrivate for testing.
 */
cr.define('chrome', function() {
  /**
   * @constructor
   * @implements {NetworkingPrivate}
   */
  function FakeNetworkingPrivate() {
    /** @type {!Object<chrome.networkingPrivate.DeviceStateProperties>} */
    this.deviceStates_ = {};

    /** @type {!Array<!CrOnc.NetworkStateProperties>} */
    this.networkStates_ = [];

    /** @type {!{chrome.networkingPrivate.GlobalPolicy}|undefined} */
    this.globalPolicy_ = undefined;

    /** @type {!{chrome.networkingPrivate.CertificateLists}|undefined} */
    this.certificateLists_ = undefined;

    this.resetForTest();
  }

  FakeNetworkingPrivate.prototype = {
    resetForTest() {
      this.deviceStates_ = {
        Ethernet: {Type: 'Ethernet', State: 'Enabled'},
        WiFi: {Type: 'WiFi', State: ''},
        Cellular: {Type: 'Cellular', State: ''},
        Tether: {Type: 'Tether', State: ''},
        WiMAX: {Type: 'WiMAX', State: ''},
      };

      this.networkStates_ = [
        {GUID: 'eth0_guid', Type: 'Ethernet'},
      ];

      this.globalPolicy_ = {};
      this.certificateLists_ = {
        serverCaCertificates: [],
        userCertificates: [],
      };
    },

    /** @param {!Array<!CrOnc.NetworkStateProperties>} network */
    addNetworksForTest: function(networks) {
      this.networkStates_ = this.networkStates_.concat(networks);
    },

    /**
     * @param {string} type
     * @return {?chrome.networkingPrivate.DeviceStateProperties}
     */
    getDeviceStateForTest: function(type) {
      return this.deviceStates_[type] || null;
    },

    /** @override */
    getProperties: function(guid, callback) {
      var result = this.networkStates_.find(function(state) {
        return state.GUID == guid;
      });
      callback(result);
    },

    /** @override */
    getManagedProperties: function(guid, callback) {
      var result = this.networkStates_.find(function(state) {
        return state.GUID == guid;
      });
      // TODO(stevenjb): Convert state to ManagedProperties.
      callback(result);
    },

    /** @override */
    getState: assertNotReached,

    /** @override */
    setProperties: assertNotReached,

    /** @override */
    createNetwork: assertNotReached,

    /** @override */
    forgetNetwork: assertNotReached,

    /** @override */
    getNetworks: function(filter, callback) {
      var type = filter.networkType;
      if (type == chrome.networkingPrivate.NetworkType.ALL) {
        callback(this.networkStates_.slice());
      } else {
        callback(this.networkStates_.filter(function(state) {
          return state.Type == type;
        }));
      }
    },

    /** @override */
    getDeviceStates: function(callback) {
      var devices = [];
      Object.keys(this.deviceStates_).forEach(function(type) {
        var state = this.deviceStates_[type];
        if (state.State != '')
          devices.push(state);
      }.bind(this));
      callback(devices);
    },

    /** @override */
    enableNetworkType: function(type) {
      this.deviceStates_[type].State = 'Enabled';
      this.onDeviceStateListChanged.callListeners();
    },

    /** @override */
    disableNetworkType: function(type) {
      this.deviceStates_[type].State = 'Disabled';
      this.onDeviceStateListChanged.callListeners();
    },

    /** @override */
    requestNetworkScan: function() {},

    /** @override */
    startConnect: assertNotReached,

    /** @override */
    startDisconnect: assertNotReached,

    /** @override */
    startActivate: assertNotReached,

    /** @override */
    verifyDestination: assertNotReached,

    /** @override */
    verifyAndEncryptCredentials: assertNotReached,

    /** @override */
    verifyAndEncryptData: assertNotReached,

    /** @override */
    setWifiTDLSEnabledState: assertNotReached,

    /** @override */
    getWifiTDLSStatus: assertNotReached,

    /** @override */
    getCaptivePortalStatus: assertNotReached,

    /** @override */
    unlockCellularSim: assertNotReached,

    /** @override */
    setCellularSimState: assertNotReached,

    /** @override */
    selectCellularMobileNetwork: assertNotReached,

    /** @override */
    getGlobalPolicy: function(callback) {
      callback(this.globalPolicy_);
    },

    /** @override */
    getCertificateLists: function(callback) {
      callback(this.certificateLists_);
    },

    /** @type {!FakeChromeEvent} */
    onNetworksChanged: new FakeChromeEvent(),

    /** @type {!FakeChromeEvent} */
    onNetworkListChanged: new FakeChromeEvent(),

    /** @type {!FakeChromeEvent} */
    onDeviceStateListChanged: new FakeChromeEvent(),

    /** @type {!FakeChromeEvent} */
    onPortalDetectionCompleted: new FakeChromeEvent(),

    /** @type {!FakeChromeEvent} */
    onCertificateListsChanged: new FakeChromeEvent(),
  };

  return {FakeNetworkingPrivate: FakeNetworkingPrivate};
});
