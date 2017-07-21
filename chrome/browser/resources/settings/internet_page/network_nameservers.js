// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for displaying network nameserver options.
 */
Polymer({
  is: 'network-nameservers',

  properties: {
    /**
     * The network properties dictionary containing the nameserver properties to
     * display and modify.
     * @type {!CrOnc.NetworkProperties|undefined}
     */
    networkProperties: {
      type: Object,
      observer: 'networkPropertiesChanged_',
    },

    /** Whether or not the nameservers can be edited. */
    editable: {
      type: Boolean,
      value: false,
    },

    /**
     * Array of nameserver addresses stored as strings.
     * @private {!Array<string>}
     */
    nameservers_: {
      type: Array,
      value: function() {
        return [];
      },
    },

    /**
     * State of 'Automatic name servers'.
     * @private
     */
    automatic_: {
      type: Boolean,
      value: true,
    },

    /**
     * State of 'Use Google name servers'.
     * @private
     */
    google_: {
      type: Boolean,
      value: false,
    },
  },

  observers: ['onTypeChange_(automatic_, google_)'],

  /** @const */
  GOOGLE_NAMESERVERS: [
    '8.8.4.4',
    '8.8.8.8',
  ],

  /** @const */
  MAX_NAMESERVERS: 4,

  /**
   * Saved nameservers when switching to 'automatic'.
   * @private {!Array<string>}
   */
  savedNameservers_: [],

  /** @private */
  networkPropertiesChanged_: function(newValue, oldValue) {
    if (!this.networkProperties)
      return;

    if (!oldValue || newValue.GUID != oldValue.GUID)
      this.savedNameservers_ = [];

    // Update the 'nameservers' property.
    var nameservers = [];
    var ipv4 =
        CrOnc.getIPConfigForType(this.networkProperties, CrOnc.IPType.IPV4);
    if (ipv4 && ipv4.NameServers)
      nameservers = ipv4.NameServers;
    this.nameservers_ = nameservers;

    // Update the |automatic_| and |google_| properties.
    var configType =
        CrOnc.getActiveValue(this.networkProperties.NameServersConfigType);
    this.google_ = nameservers.join(',') == this.GOOGLE_NAMESERVERS.join(',');
    this.automatic_ = configType != CrOnc.IPConfigType.STATIC;
  },

  /** @private */
  onTypeChange_: function() {
    if (!this.automatic_ && this.google_) {
      this.nameservers_ = this.GOOGLE_NAMESERVERS;
    }
    if (!this.automatic_ && !this.google_) {
      var nameservers = this.savedNameservers_.slice();
      // Add empty entries for unset custom nameservers.
      for (var i = nameservers.length; i < this.MAX_NAMESERVERS; ++i)
        nameservers[i] = '';
      this.nameservers_ = nameservers;
    }
    this.sendNameServers_();
  },

  /**
   * @param {boolean} editable
   * @param {boolean} automatic
   * @param {boolean} google
   * @return {boolean} True if the nameservers are editable.
   * @private
   */
  canEdit_: function(editable, automatic, google) {
    return editable && !automatic && !google;
  },

  /**
   * Event triggered when a nameserver value changes.
   * @private
   */
  onValueChange_: function() {
    var nameservers = new Array(this.MAX_NAMESERVERS);
    for (var i = 0; i < this.MAX_NAMESERVERS; ++i) {
      var nameserverInput = this.$$('#nameserver' + i);
      nameservers[i] = nameserverInput ? nameserverInput.value : '';
    }
    this.nameservers_ = nameservers;
    this.savedNameservers_ = this.nameservers_;
    this.sendNameServers_();
  },

  /**
   * Sends the current nameservers type (for automatic) or value.
   * @private
   */
  sendNameServers_: function() {
    if (this.automatic_) {
      this.fire('nameservers-change', {
        field: 'NameServersConfigType',
        value: CrOnc.IPConfigType.DHCP,
      });
      return;
    }

    if (this.google_) {
      this.fire('nameservers-change', {
        field: 'NameServers',
        value: this.GOOGLE_NAMESERVERS,
      });
      return;
    }

    // Custom nameservers
    var hasValue = false;
    for (var i = 0; i < this.MAX_NAMESERVERS; ++i)
      hasValue |= !!this.nameservers_[i];
    if (!hasValue)
      return;  // Don't send or save empty nameservers
    this.fire('nameservers-change', {
      field: 'NameServers',
      value: this.nameservers_.slice(),
    });
  },
});
