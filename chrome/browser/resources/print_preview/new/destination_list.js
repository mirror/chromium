// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {
'use strict';

/** @type {number} */
const SHORT_DESTINATION_LIST_SIZE = 4;

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

    /** @private {boolean} */
    showAll_: {
      type: Boolean,
      value: false,
    },

    /** @private {number} */
    matchingDestinationsCount_: Number,

    /** @type {boolean} */
    footerHidden_: {
      type: Boolean,
      computed: 'computeFooterHidden_(matchingDestinationsCount_, showAll_)',
    },

    /** @private {boolean} */
    hasDestinations_: {
      type: Boolean,
      computed: 'computeHasDestinations_(matchingDestinationsCount_)',
    },
  },

  /**
   * @return {?Function}
   * @private
   */
  computeFilter_: function() {
    return !!this.searchQuery ?
        destination => destination.matches(this.searchQuery) :
        null;
  },

  /**
   * @return {boolean}
   * @private
   */
  computeFooterHidden_: function() {
    return this.matchingDestinationsCount_ < SHORT_DESTINATION_LIST_SIZE ||
        this.showAll_;
  },

  /**
   * @return {boolean}
   * @private
   */
  computeHasDestinations_: function() {
    return this.matchingDestinationsCount_ > 0;
  },

  /**
   * @param {number} index The index of the destination in the list.
   * @return {boolean} Whether the destination should be hidden
   * @private
   */
  isDestinationHidden_: function(index) {
    return !this.showAll_ && index >= SHORT_DESTINATION_LIST_SIZE;
  },

  /**
   * @param {boolean} offlineOrInvalid Whether the destination is offline or
   *     invalid
   * @return {string} An empty string or 'stale'.
   * @private
   */
  getStaleCssClass_: function(offlineOrInvalid) {
    return offlineOrInvalid ? 'stale' : '';
  },

  /** @private */
  onActionLinkTap_: function() {
    print_preview.NativeLayer.getInstance().managePrinters();
  },

  /** @private */
  onShowAllTap_: function() {
    this.showAll_ = true;
  },

  /**
   * @param {!Event} e Event containing the destination that was selected.
   * @private
   */
  onDestinationSelected_: function(e) {
    this.fire(
        'destination-selected', /** @type {!{model: Object}} */ (e).model.item);
  },

  reset: function() {
    this.showAll_ = false;
  },
});
})();
