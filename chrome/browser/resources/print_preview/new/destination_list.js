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
    numMatchingDestinations_: {
      type: Number,
      computed: 'computeNumMatchingDestinations_(destinations, searchQuery)',
    },

    /** @private {number} */
    lastDisplayedIndex_: {
      type: Number,
      computed: 'computeLastDisplayedIndex_(' +
          'destinations, numMatchingDestinations_, showAll_, searchQuery)',
    },

    /** @type {boolean} */
    footerHidden_: {
      type: Boolean,
      computed: 'computeFooterHidden_(numMatchingDestinations_, showAll_)',
    },

    /** @private {boolean} */
    hasDestinations_: {
      type: Boolean,
      computed: 'computeHasDestinations_(numMatchingDestinations_)',
    },
  },

  observers: [
    'highlightListItems_(lastDisplayedIndex_, searchQuery)',
  ],

  /**
   * @return {number}
   * @private
   */
  computeNumMatchingDestinations_: function() {
    if (!this.searchQuery)
      return assert(this.destinations).length;
    return this.destinations
        .filter(destination => {
          return destination.matches(assert(this.searchQuery));
        })
        .length;
  },

  /**
   * @param {string} property The property of the destination that should be
   *     displayed if it matches the current search query.
   * @return {boolean} Whether the property should be hidden.
   * @private
   */
  isPropertyHidden_: function(property) {
    if (!this.searchQuery)
      return true;
    return !property.match(this.searchQuery);
  },

  /**
   * @return {boolean}
   * @private
   */
  computeFooterHidden_: function() {
    return this.numMatchingDestinations_ < SHORT_DESTINATION_LIST_SIZE ||
        this.showAll_;
  },

  /**
   * @return {boolean}
   * @private
   */
  computeHasDestinations_: function() {
    return this.numMatchingDestinations_ > 0;
  },

  /**
   * @return {number}
   * @private
   */
  computeLastDisplayedIndex_: function() {
    if (this.showAll_) {
      return this.destinations.length;
    }

    const numToDisplay =
        Math.min(this.numMatchingDestinations_, SHORT_DESTINATION_LIST_SIZE);
    if (!this.searchQuery)
      return numToDisplay - 1;

    let startIndex = 0;
    for (let i = 0; i < numToDisplay; i++) {
      let newIndex =
          this.destinations.slice(startIndex).findIndex(destination => {
            return destination.matches(assert(this.searchQuery));
          });
      startIndex += newIndex + 1;
    }
    return startIndex - 1;
  },

  /**
   * @param {!print_preview.Destination} destination The destination.
   * @param {number} index The index of the destination in the list.
   * @return {boolean} Whether the destination should be hidden
   * @private
   */
  isDestinationHidden_: function(destination, index) {
    return index > this.lastDisplayedIndex_ ||
        (!!this.searchQuery && !destination.matches(this.searchQuery));
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

  /** @private */
  highlightListItems_: function() {
    findAndRemoveHighlights(this);
    if (!this.searchQuery)
      return;
    this.shadowRoot.querySelectorAll('.searchable').forEach(element => {
      element.childNodes.forEach(node => {
        if (node.nodeType != Node.TEXT_NODE)
          return;

        const textContent = node.nodeValue.trim();
        if (textContent.length == 0)
          return;

        if (this.searchQuery.test(textContent)) {
          // Don't highlight <select> nodes, yellow rectangles can't be
          // displayed within an <option>.
          // TODO(rbpotter): solve issue below before adding advanced
          // settings so that this function can be re-used.
          // TODO(dpapad): highlight <select> controls with a search bubble
          // instead.
          if (node.parentNode.nodeName != 'OPTION')
            highlight(node, textContent.split(this.searchQuery));
        }
      });
    });
  },

  reset: function() {
    this.showAll_ = false;
  },
});
})();
