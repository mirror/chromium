// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'Settings page for managing bluetooth properties and devices. This page
 * just provodes a summary and link to the subpage.
 */

const bluetoothApis = window['bluetoothApis'] || {
  /**
   * Set this to provide a fake implementation for testing.
   * @type {Bluetooth}
   */
  bluetoothApiForTest: null,

  /**
   * Set this to provide a fake implementation for testing.
   * @type {BluetoothPrivate}
   */
  bluetoothPrivateApiForTest: null,
};

/**
 * The time in ms to delay updating the toggle button UI.
 * @type {number}
 */
const TOGGLE_DEBOUNCE_MS = 500;

Polymer({
  is: 'settings-bluetooth-page',

  behaviors: [PrefsBehavior],

  properties: {
    /** Preferences state. */
    prefs: {
      type: Object,
      notify: true,
    },

    /**
     * Reflects the current state of the toggle buttons (in this page and the
     * subpage). This will be set when the adapter state change or when the user
     * changes the toggle.
     * @private
     */
    bluetoothToggleState_: {
      type: Boolean,
      observer: 'bluetoothToggleStateChanged_',
    },

    /**
     * The cached bluetooth adapter state.
     * @type {!chrome.bluetooth.AdapterState|undefined}
     * @private
     */
    adapterState_: {
      type: Object,
      notify: true,
    },

    /** @private {!Map<string, string>} */
    focusConfig_: {
      type: Object,
      value: function() {
        const map = new Map();
        if (settings.routes.BLUETOOTH_DEVICES) {
          map.set(
              settings.routes.BLUETOOTH_DEVICES.path,
              '#bluetoothDevices .subpage-arrow');
        }
        return map;
      },
    },

    /**
     * Interface for bluetooth calls. May be overriden by tests.
     * @type {Bluetooth}
     * @private
     */
    bluetooth: {
      type: Object,
      value: chrome.bluetooth,
    },

    /**
     * Interface for bluetoothPrivate calls. May be overriden by tests.
     * @type {BluetoothPrivate}
     * @private
     */
    bluetoothPrivate: {
      type: Object,
      value: chrome.bluetoothPrivate,
    },

    /**
     * Whether we should update pref value of
     * ash.user.bluetooth.adapter_enabled. Default value is true, and this is
     * set to false when we get bluetooth adapter state changed event to avoid
     * infinite loop.
     * @private
     */
    shouldSetPref_: {
      type: Boolean,
      value: true,
    },

    /**
     * Used by tests to update toggle state immediately upon adapter state
     * change event
     * @type {boolean}
     */
    immediatelyUpdateAdapterState: {
      type: Boolean,
      value: false,
    },
  },

  observers: ['deviceListChanged_(deviceList_.*)'],

  /**
   * Listener for chrome.bluetooth.onAdapterStateChanged events.
   * @type {function(!chrome.bluetooth.AdapterState)|undefined}
   * @private
   */
  bluetoothAdapterStateChangedListener_: undefined,

  /** @override */
  ready: function() {
    if (bluetoothApis.bluetoothApiForTest)
      this.bluetooth = bluetoothApis.bluetoothApiForTest;
    if (bluetoothApis.bluetoothPrivateApiForTest)
      this.bluetoothPrivate = bluetoothApis.bluetoothPrivateApiForTest;
  },

  /** @override */
  attached: function() {
    this.bluetoothAdapterStateChangedListener_ =
        this.onBluetoothAdapterStateChanged_.bind(this);
    this.bluetooth.onAdapterStateChanged.addListener(
        this.bluetoothAdapterStateChangedListener_);

    // Request the inital adapter state.
    this.bluetooth.getAdapterState(this.bluetoothAdapterStateChangedListener_);
  },

  /** @override */
  detached: function() {
    if (this.bluetoothAdapterStateChangedListener_) {
      this.bluetooth.onAdapterStateChanged.removeListener(
          this.bluetoothAdapterStateChangedListener_);
    }
  },

  /**
   * @return {string}
   * @private
   */
  getIcon_: function() {
    if (!this.bluetoothToggleState_)
      return 'settings:bluetooth-disabled';
    return 'settings:bluetooth';
  },

  /**
   * @param {boolean} enabled
   * @param {string} onstr
   * @param {string} offstr
   * @return {string}
   * @private
   */
  getOnOffString_: function(enabled, onstr, offstr) {
    return enabled ? onstr : offstr;
  },

  /**
   * Process bluetooth.onAdapterStateChanged events.
   * @param {!chrome.bluetooth.AdapterState} state
   * @private
   */
  onBluetoothAdapterStateChanged_: function(state) {
    this.adapterState_ = state;
    this.delayUpdateToggleState_();
  },

  /** @private */
  onTap_: function() {
    if (this.adapterState_.available === false)
      return;
    if (!this.bluetoothToggleState_)
      this.bluetoothToggleState_ = true;
    else
      this.openSubpage_();
  },

  /**
   * @param {!Event} e
   * @private
   */
  stopTap_: function(e) {
    e.stopPropagation();
  },

  /**
   * @param {!Event} e
   * @private
   */
  onSubpageArrowTap_: function(e) {
    this.openSubpage_();
    e.stopPropagation();
  },

  /** @private */
  bluetoothToggleStateChanged_: function() {
    if (!this.adapterState_)
      return;

    if (this.shouldSetPref_ === false) {
      this.shouldSetPref_ = true;
      return;
    }

    this.setPrefValue(
        'ash.user.bluetooth.adapter_enabled', this.bluetoothToggleState_);
  },

  /** @private */
  openSubpage_: function() {
    settings.navigateTo(settings.routes.BLUETOOTH_DEVICES);
  },

  /** @private */
  updateToggleState_: function() {
    if (this.adapterState_ &&
        this.adapterState_.powered != this.bluetoothToggleState_) {
      this.shouldSetPref_ = false;
      this.bluetoothToggleState_ = this.adapterState_.powered;
    }
  },

  /** @private */
  delayUpdateToggleState_: function() {
    if (this.immediatelyUpdateAdapterState) {
      this.updateToggleState_();
      return;
    }
    setTimeout(() => {
      this.updateToggleState_();
    }, TOGGLE_DEBOUNCE_MS);
  }
});
