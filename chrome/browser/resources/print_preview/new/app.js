// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-app',

  behaviors: [SettingsBehavior],
  properties: {
    /**
     * Object containing current settings of Print Preview, for use by Polymer
     * controls.
     * @type {!Object}
     */
    settings: {
      type: Object,
      notify: true,
    },

    /** @private {print_preview.DocumentInfo} */
    documentInfo_: {
      type: Object,
      notify: true,
    },

    /** @private {print_preview.Destination} */
    destination_: {
      type: Object,
      notify: true,
    },

    /** @private {!print_preview_new.State} */
    state_: {
      type: Object,
      notify: true,
      value: {
        previewLoading: false,
        previewFailed: false,
        cloudPrintError: '',
        privetExtensionError: '',
        invalidSettings: false,
      },
    },
  },

  observers: [
    'updateRecentDestinations_(destination_, destination_.capabilities)',
  ],

  /**
   * @private {number} Number of recent destinations to save.
   * @const
   */
  NUM_DESTINATIONS_: 3,

  /** @private {?print_preview.NativeLayer} */
  nativeLayer_: null,

  /** @private {?print_preview.UserInfo} */
  userInfo_: null,

  /** @private {?WebUIListenerTracker} */
  listenerTracker_: null,

  /** @private {?print_preview.DestinationStore} */
  destinationStore_: null,

  /** @private {!EventTracker} */
  tracker_: new EventTracker(),

  /** @type {!print_preview.MeasurementSystem} */
  measurementSystem_: new print_preview.MeasurementSystem(
      ',', '.', print_preview.MeasurementSystemUnitType.IMPERIAL),

  /** @override */
  attached: function() {
    this.nativeLayer_ = print_preview.NativeLayer.getInstance(),
    this.documentInfo_ = new print_preview.DocumentInfo();
    this.userInfo_ = new print_preview.UserInfo();
    this.listenerTracker_ = new WebUIListenerTracker();
    this.destinationStore_ = new print_preview.DestinationStore(
        this.userInfo_, this.listenerTracker_);
    this.tracker_.add(
        this.destinationStore_,
        print_preview.DestinationStore.EventType.DESTINATION_SELECT,
        this.onDestinationSelect_.bind(this));
    this.tracker_.add(
        this.destinationStore_,
        print_preview.DestinationStore.EventType
            .SELECTED_DESTINATION_CAPABILITIES_READY,
        this.onDestinationUpdated_.bind(this));
    this.nativeLayer_.getInitialSettings().then(
        this.onInitialSettingsSet_.bind(this));
  },

  /** @override */
  detached: function() {
    this.listenerTracker_.removeAll();
    this.tracker_.removeAll();
  },

  /**
   * @param {!print_preview.NativeInitialSettings} settings
   * @private
   */
  onInitialSettingsSet_: function(settings) {
    this.documentInfo_.init(
        settings.previewModifiable, settings.documentTitle,
        settings.documentHasSelection);
    // Temporary setting, will be replaced when page count is known from
    // the page-count-ready webUI event.
    this.documentInfo_.updatePageCount(5);
    this.notifyPath('documentInfo_.isModifiable');
    // Before triggering the final notification for settings availability,
    // set initialized = true.
    this.notifyPath('documentInfo_.hasSelection');
    this.notifyPath('documentInfo_.title');
    this.notifyPath('documentInfo_.pageCount');
    this.updateFromStickySettings_(settings.serializedAppStateStr);
    this.measurementSystem_.setSystem(
        settings.thousandsDelimeter, settings.decimalDelimeter,
        settings.unitType);
    this.setSetting('selectionOnly', settings.shouldPrintSelectionOnly);
    this.destinationStore_.init(
        settings.isInAppKioskMode, settings.printerName,
        settings.serializedDefaultDestinationSelectionRulesStr,
        this.settings.serialization.recentDestinations);
  },

  /** @private */
  onDestinationSelect_: function() {
    this.destination_ = this.destinationStore_.selectedDestination;
  },

  /** @private */
  onDestinationUpdated_: function() {
    this.set(
        'destination_.capabilities',
        this.destinationStore_.selectedDestination.capabilities);
  },

  /** @private */
  updateRecentDestinations_: function() {
    if (!this.destination_)
      return;

    const recentDestinations = this.settings.serialization.recentDestinations;
    // Determine if this destination is already in the recent destinations,
    // and where in the array it is located.
    const newDestination =
        print_preview.makeRecentDestination(assert(this.destination_));
    let indexFound = recentDestinations.findIndex(function(recent) {
      return (
          newDestination.id == recent.id &&
          newDestination.origin == recent.origin);
    });

    // No change
    if (indexFound == 0 &&
        recentDestinations[0].capabilities == newDestination.capabilities) {
      return;
    }

    // Shift the array so that the nth most recent destination is located at
    // index n.
    if (indexFound == -1 &&
        recentDestinations.length == this.NUM_DESTINATIONS_) {
      indexFound = this.NUM_DESTINATIONS_ - 1;
    }
    if (indexFound != -1)
      this.settings.serialization.recentDestinations.splice(indexFound, 1);

    // Add the most recent destination
    this.settings.serialization.recentDestinations.splice(0, 0, newDestination);
  },

  /**
   * @param {?string} savedSettingsStr The sticky settings from native layer
   * @private
   */
  updateFromStickySettings_(savedSettingsStr) {
    if (!savedSettingsStr) {
      this.settings.serialization.version = 2;
      this.settings.serialization.recentDestinations = [];
      this.settings.initialized = true;
      return;
    }

    let savedSettings;
    try {
      savedSettings = /** @type {print_preview_new.SerializedSettings} */ (
          JSON.parse(savedSettingsStr));
    } catch (e) {
      console.error('Unable to parse state ' + e);
      this.settings.initialized = true;
      return;  // use default values rather than updating.
    }
    if (savedSettings.version == 2)
      this.set('settings.serialization', savedSettings);

    this.settings.serialization.recentDestinations =
        this.settings.serialization.recentDestinations || [];
    if (!Array.isArray(this.settings.serialization.recentDestinations)) {
      this.settings.serialization.recentDestinations =
          [this.settings.serialization.recentDestinations];
    }

    ['dpi', 'mediaSize', 'margins', 'color', 'headerFooter', 'layout',
     'collate', 'scaling', 'fitToPage', 'duplex', 'cssBackground']
        .forEach(settingName => {
          const setting = this.getSetting(settingName);
          if (this.settings.serialization[setting.key] != undefined) {
            this.setSetting(
                settingName, this.settings.serialization[setting.key]);
          }
        });
    this.settings.initialized = true;
  },
});
