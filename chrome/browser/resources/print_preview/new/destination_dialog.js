// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-destination-dialog',

  properties: {
    /** @private {Array<!print_preview.Destination>} */
    destinations_: {
      type: Array,
      notify: true,
      value: [],
    },

    /** @type {!print_preview.DestinationStore} */
    destinationStore: {
      type: Object,
      observer: 'onDestinationStoreSet_',
    },

    /** @private {boolean} */
    loadingDestinations_: {
      type: Boolean,
      value: false,
    },

    /** @type {!print_preview.UserInfo} */
    userInfo: Object,
  },

  tracker_: new EventTracker(),

  /** @private */
  updateDestinations_: function() {
    this.destinations = (this.userInfo && this.destinationStore) ?
        this.destinationStore.destinations(this.userInfo.activeUser) :
        [];
    this.loadingDestinations_ =
        this.destinationStore.isPrintDestinationSearchInProgress;
  },

  /** @private */
  onDestinationStoreSet_: function() {
    if (!this.destinationStore)
      return;
    this.tracker_.add(
        this.destinationStore,
        print_preview.DestinationStore.EventType.DESTINATIONS_INSERTED,
        this.updateDestinations_.bind(this));
    this.tracker_.add(
        this.destinationStore,
        print_preview.DestinationStore.EventType.DESTINATION_SEARCH_DONE,
        this.updateDestinations_.bind(this));
  },

  show: function() {
    this.loadingDestinations_ = true;
    if (!this.$.dialog.open)
      this.$.dialog.showModal();
    this.changeButtonTapped = false;
  },

  /** @private */
  onCancelButtonTap_: function() {
    this.$.dialog.cancel();
  },
});
