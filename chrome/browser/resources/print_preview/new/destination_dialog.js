// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-destination-dialog',

  properties: {
    /** @type {?print_preview.DestinationStore} */
    destinationStore: {
      type: Object,
      observer: 'onDestinationStoreSet_',
    },

    /** @type {!print_preview.UserInfo} */
    userInfo: Object,

    /** @private {!Array<!print_preview.Destination>} */
    destinations_: {
      type: Array,
      notify: true,
      value: [],
    },

    /** @private {boolean} */
    loadingDestinations_: {
      type: Boolean,
      value: false,
    },

    /** @type {!Array<!print_preview.RecentDestination>} */
    recentDestinations: Array,

    /** @private {!Array<!print_preview.Destination>} */
    recentDestinationList_: {
      type: Array,
      notify: true,
      computed: 'computeRecentDestinationList_(' +
          'destinationStore, recentDestinations, recentDestinations.*, ' +
          'userInfo, destinations_.*)',
    },

    /** @private {?RegExp} */
    searchQuery_: {
      type: Object,
      value: null,
    },
  },

  /** @private {!EventTracker} */
  tracker_: new EventTracker(),

  /** @private */
  onDestinationStoreSet_: function() {
    assert(this.destinations_.length == 0);
    const destinationStore = assert(this.destinationStore);
    this.tracker_.add(
        destinationStore,
        print_preview.DestinationStore.EventType.DESTINATIONS_INSERTED,
        this.updateDestinations_.bind(this));
    this.tracker_.add(
        destinationStore,
        print_preview.DestinationStore.EventType.DESTINATION_SEARCH_DONE,
        this.updateDestinations_.bind(this));
  },

  /** @private */
  updateDestinations_: function() {
    this.destinations_ = (this.userInfo && this.destinationStore) ?
        this.destinationStore.destinations(this.userInfo.activeUser) :
        [];
    this.loadingDestinations_ =
        this.destinationStore.isPrintDestinationSearchInProgress;
  },

  /**
   * @return {!Array<!print_preview.Destination>}
   * @private
   */
  computeRecentDestinationList_: function() {
    let recentDestinations = [];
    const filterAccount = this.userInfo.activeUser;
    this.recentDestinations.forEach((recentDestination) => {
      const origin = recentDestination.origin;
      const id = recentDestination.id;
      const account = recentDestination.account || '';
      const destination =
          this.destinationStore.getDestination(origin, id, account);
      if (destination &&
          (!destination.account || destination.account == filterAccount)) {
        recentDestinations.push(destination);
      }
    });
    return recentDestinations;
  },

  /**
   * @return {string} The cloud print promotion HTML.
   * @private
   */
  getCloudPrintPromotion_: function() {
    return loadTimeData.getStringF(
        'cloudPrintPromotion', '<a is="action-link" class="sign-in">', '</a>');
  },

  /** @private */
  onCancel_: function() {
    this.$$('.search-box-input').value = '';
    this.searchQuery_ = null;
    Polymer.dom(this.root)
        .querySelectorAll('print-preview-destination-list')
        .forEach(list => list.reset());
    this.$.dialog.cancel();
  },

  show: function() {
    this.loadingDestinations_ =
        this.destinationStore.isPrintDestinationSearchInProgress;
    this.$.dialog.showModal();
  },

  /**
   * @param {!CustomEvent} e Event containing the selected destination.
   * @private
   */
  onDestinationSelected_: function(e) {
    this.destinationStore.selectDestination(
        /** @type {!print_preview.Destination} */ (e.detail));
    this.onCancel_();
  },

  onSearchBoxInput_: function() {
    const query = this.$$('.search-box-input').value.trim();
    if (query) {
      const safeQuery = query.replace(/[-[\]{}()*+?.,\\^$|#\s]/g, '\\$&');
      this.searchQuery_ = new RegExp('(' + safeQuery + ')', 'ig');
    } else {
      this.searchQuery_ = null;
    }
  },
});
