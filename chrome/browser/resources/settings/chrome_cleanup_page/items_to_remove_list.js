// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

settings.NUM_ITEMS_TO_SHOW = 4;

/**
 * @fileoverview
 * 'items-to-remove-list' represents a list of itmes to
 * be removed or changed to be shown on the Chrome Cleanup page.
 *
 * Examples:
 *
 *    <!-- Items list initially expanded. -->
 *    <items-to-remove-list
 *        title="Files and programs:"
 *        expanded="true"
 *        items-to-show="[[filesToShow]]">
 *    </items-to-remove-list>
 *
 *    <!-- Items list initially shows |NUM_ITEMS_TO_SHOW| items. If there are
 *         more than |NUM_ITEMS_TO_SHOW| items on the list, then a "show more"
 *         link is shown; tapping on it expands the list. -->
 *    <items-to-remove-list
 *        title="Files and programs:"
 *        items-to-show="[[filesToShow]]">
 *    </items-to-remove-list>
 */
Polymer({
  is: 'items-to-remove-list',

  properties: {
    titleVisible: {
      type: Boolean,
      value: true,
    },

    title: {
      type: String,
      value: '',
    },

    /** @type {Array<string>} */
    itemsToShow: {
      type: Array,
      value: function() {
        return [];
      },
    },

    /**
     * If true, all items from |itemsToShow| will be presented on the card,
     * and the "show more" link will be omitted.
     */
    expanded: {
      type: Boolean,
      value: false,
    },

    /*
     * The list of items to actually present on the card. If |expanded|, then
     * it's the same as |itemsToShow|.
     * @private {Array<string>}
     */
    visibleItems_: {
      type: Array,
      notify: true,
      value: function() {
        return [];
      },
    },

    /**
     * The text for the "show more" link available if not all files are visible
     * in the card.
     * @private
     */
    moreItemsLinkText_: {
      type: String,
      value: '',
    },
  },

  observers: ['updateVisibleState_(itemsToShow, expanded)'],

  /** @private {?settings.ChromeCleanupProxy} */
  browserProxy_: null,

  /** @private */
  expandList_: function() {
    this.expanded = true;
    this.visibleItems_ = this.itemsToShow;
    this.moreItemsLinkText_ = '';
  },

  /**
   * Decides if which elements will be visible in the card and if the
   * "show more" link will be rendered.
   *
   * Cases handled:
   *  1. If |expanded|, then all items will be visible.
   *  2. Otherwise:
   *     (A) If size(itemsToShow) < NUM_ITEMS_TO_SHOW, then all items will be
   *         visible.
   *     (B) Otherwise, exactly |NUM_ITEMS_TO_SHOW - 1| will be visible and
   *         the "show more" link will be rendered. The list presented to the
   *         user will contain exactly |NUM_ITEMS_TO_SHOW| elements, and the
   *         last one will be the "show more" link.
   * @param {!Array<string>} itemsToShow
   * @param {boolean} expanded
   */
  updateVisibleState_: function(itemsToShow, expanded) {
    if (this.itemsToShow.length == 0)
      return;

    // Start expanded if there are less than |settings.NUM_ITEMS_TO_SHOW| items
    // to show.
    if (!this.expanded && this.itemsToShow.length > 0 &&
        this.itemsToShow.length <= settings.NUM_ITEMS_TO_SHOW) {
      this.expanded = true;
    }

    if (this.expanded) {
      this.visibleItems_ = this.itemsToShow;
      this.updateMoreItemsLinkText_('');
      return;
    }

    this.visibleItems_ =
        this.itemsToShow.slice(0, settings.NUM_ITEMS_TO_SHOW - 1);

    if (!this.browserProxy_)
      this.browserProxy_ = settings.ChromeCleanupProxyImpl.getInstance();
    this.browserProxy_
        .getMoreItemsPluralString(
            this.itemsToShow.length - this.visibleItems_.length)
        .then(this.updateMoreItemsLinkText_.bind(this));
  },

  updateMoreItemsLinkText_: function(linkTest) {
    this.moreItemsLinkText_ = linkTest;
  },
});
