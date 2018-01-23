// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-destination-settings',

  properties: {
    /** @type {!print_preview.Destination} */
    destination: Object,

    /** @type {!print_preview.DestinationStore} */
    destinationStore: {
      type: Object,
      observer: 'onDestinationStoreSet_',
    },

    /** @private {Array<!print_preview.Destination>} */
    destinations_: {
      type: Array,
      notify: true,
      value: [],
    },

    /** @type {!print_preview.UserInfo} */
    userInfo: Object,

    /** @private {boolean} */
    loadingDestination_: {
      type: Boolean,
      value: true,
    },
  },

  observers: ['onDestinationSet_(destination, destination.id)'],

  tracker_: new EventTracker(),

  updateDestinations_: function() {
    this.destinations_ = (this.userInfo && this.destinationStore) ?
        this.destinationStore.destinations(this.userInfo.activeUser) :
        [];
  },

  onDestinationStoreSet_: function() {
    if (this.destinationStore) {
      this.tracker_.add(
          this.destinationStore,
          print_preview.DestinationStore.EventType.DESTINATIONS_INSERTED,
          this.updateDestinations_.bind(this));
      this.tracker_.add(
          this.destinationStore,
          print_preview.DestinationStore.EventType.DESTINATION_SEARCH_DONE,
          this.updateDestinations_.bind(this));
    }
  },

  /** @private */
  onDestinationSet_: function() {
    if (this.destination && this.destination.id)
      this.loadingDestination_ = false;
  },

  onChangeButtonTap_: function() {
    if (!this.destinationStore)
      return;
    this.destinationStore.startLoadAllDestinations();
    const dialog = this.$.destinationDialog.get();
    this.async(() => {
      dialog.show();
    }, 1);
  },
});
