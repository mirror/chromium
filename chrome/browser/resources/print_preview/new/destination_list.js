// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
'use strict';

Polymer({
  is: 'print-preview-destination-list',

  behaviors: [I18nBehavior],

  properties: {
    /** @type {Array<!print_preview.Destination>} */
    destinations: Array,

    /** @type {boolean} */
    hasActionLink: {
      type: Boolean,
      value: false,
    },

    /** @type {boolean} */
    loadingDestinations: {
      type: Boolean,
      value: false,
    },

    /** @type {?RegExp} */
    searchQuery: Object,

    /** @type {boolean} */
    title: String,

    /** @private {number} */
    matchingDestinationsCount_: Number,

    /** @private {boolean} */
    hasDestinations_: {
      type: Boolean,
      computed: 'computeHasDestinations_(matchingDestinationsCount_)',
    },

    /** @private {boolean} */
    showTotalDestinations_: {
      type: Boolean,
      computed: 'computeShowTotalDestinations_(matchingDestinationsCount_)',
    },
  },

  listeners: {
    'dom-change': 'updateListItems_',
  },

  /** @private */
  updateListItems_: function() {
    this.shadowRoot
        .querySelectorAll('print-preview-destination-list-item:not([hidden])')
        .forEach(item => item.update(this.searchQuery));
  },

  /**
   * @return {Function}
   * @private
   */
  computeFilter_: function() {
    return destination => {
      return !this.searchQuery || destination.matches(this.searchQuery);
    };
  },

  /**
   * @return {boolean}
   * @private
   */
  computeHasDestinations_: function() {
    return !this.destinations || this.matchingDestinationsCount_ > 0;
  },

  /**
   * @return {boolean}
   * @private
   */
  computeShowTotalDestinations_: function() {
    return !!this.destinations && this.matchingDestinationsCount_ > 4;
  },

  /** @private */
  onActionLinkTap_: function() {
    print_preview.NativeLayer.getInstance().managePrinters();
  },

  /**
   * @param {!Event} e Event containing the destination that was selected.
   * @private
   */
  onDestinationSelected_: function(e) {
    this.fire(
        'destination-selected', /** @type {!{model: Object}} */ (e).model.item);
  },

  reset: function() {}
});
})();
