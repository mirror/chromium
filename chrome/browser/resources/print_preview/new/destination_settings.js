// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-destination-settings',

  properties: {
    /** @type {!print_preview_new.Model} */
    model: {
      type: Object,
      notify: true,
    },

    /** @private {boolean} */
    loadingDestination_: Boolean,

    /**
     * @private {{src: string,
     *            srcSet: string}}
     */
    icon_: {
      type: Object,
      computed: 'computeIcon_(model.destination.id, ' +
          'model.destination.isEnterprisePrinter, ' +
          'model.destination.origin, ' +
          'model.destination.connectionStatus, ' +
          'model.destination.type, ' +
          'model.destination.isOwned)',
    },
  },

  /**
   * The original (complete) CDD. Stored for testing, remove later.
   * @private {?print_preview.Cdd}
   */
  originalCapabilities_: null,

  /**
   * Number of times change button has been clicked. Used to track what
   * capabilities should change to for test. Remove later.
   * @private {number}
   */
  changeCount: 0,

  /** @override */
  ready: function() {
    this.loadingDestination_ = true;
    this.originalCapabilities_ = this.model.destination.capabilities;
    // Simulate transition from spinner to destination.
    setTimeout(this.doneLoading_.bind(this), 5000);
  },

  /**
   * @return {{src: string,
   *           srcSet: string}}
   * @private
   */
  computeIcon_: function() {
    if (this.model.destination.id ==
        print_preview.Destination.GooglePromotedId.DOCS) {
      return {src: print_preview.Destination.IconUrl.DOCS, srcSet: ''};
    }
    if (this.model.destination.id ==
        print_preview.Destination.GooglePromotedId.SAVE_AS_PDF) {
      return {src: print_preview.Destination.IconUrl.PDF, srcSet: ''};
    }
    if (this.model.destination.isEnterprisePrinter) {
      return {src: print_preview.Destination.IconUrl.ENTERPRISE, srcSet: ''};
    }
    if (print_preview_new.DestinationIsLocal(this.model.destination)) {
      return {
        src: print_preview.Destination.IconUrl.LOCAL_1X,
        srcSet: print_preview.Destination.IconUrl.LOCAL_2X + ' 2x'
      };
    }
    if (this.model.destination.type == print_preview.DestinationType.MOBILE &&
        this.model.destination.isOwned) {
      return {src: print_preview.Destination.IconUrl.MOBILE, srcSet: ''};
    }
    if (this.model.destination.type == print_preview.DestinationType.MOBILE) {
      return {src: print_preview.Destination.IconUrl.MOBILE_SHARED, srcSet: ''};
    }
    if (this.model.destination.isOwned) {
      return {
        src: print_preview.Destination.IconUrl.CLOUD_1X,
        srcSet: print_preview.Destination.IconUrl.CLOUD_2X + ' 2x'
      };
    }
    return {
      src: print_preview.Destination.IconUrl.CLOUD_SHARED_1X,
      srcSet: print_preview.Destination.IconUrl.CLOUD_SHARED_2X + ' 2x'
    };

  },

  /** @private */
  doneLoading_: function() {
    this.loadingDestination_ = false;
  },

  /**
   * @return {string}
   * @private
   */
  getOfflineStatus_: function() {
    const isOffline = this.model.destination.connectionStatus ==
            print_preview.DestinationConnectionStatus.OFFLINE ||
        this.model.destination.connectionStatus ==
            print_preview.DestinationConnectionStatus.DORMANT;
    if (!isOffline)
      return '';
    const offlineDurationMs =
        Date.now() - this.model.destination.lastAccessTime;
    let offlineMessageId;
    if (offlineDurationMs > 31622400000.0) {  // One year.
      offlineMessageId = 'offlineForYear';
    } else if (offlineDurationMs > 2678400000.0) {  // One month.
      offlineMessageId = 'offlineForMonth';
    } else if (offlineDurationMs > 604800000.0) {  // One week.
      offlineMessageId = 'offlineForWeek';
    } else {
      offlineMessageId = 'offline';
    }
    return loadTimeData.getString(offlineMessageId);
  },

  /**
   * @return {string}
   * @private
   */
  getHint_: function() {
    if (this.model.destination.id ==
        print_preview.Destination.GooglePromotedId.DOCS) {
      return this.model.destination.account;
    }
    return this.model.destination.location ||
        this.model.destination.extensionName ||
        this.model.destination.description;
  },

  /**
   * Testing function for checking settings sections appearance/disappearance.
   * Replace with real implementation later.
   * @private
   */
  onButtonClick_: function() {
    const newCapabilities =
        JSON.parse(JSON.stringify(this.originalCapabilities_));
    if (this.changeCount == 0) {
      newCapabilities.printer.collate = null;
    } else if (this.changeCount == 1) {
      newCapabilities.printer.copies = null;
    } else if (this.changeCount == 2) {
      newCapabilities.printer.color = null;
    }
    this.set('model.destination.capabilities', newCapabilities);
    this.changeCount++;
    if (this.changeCount == 4)
      this.changeCount = 0;
  },
});
