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

    /** @private {!Array<print_preview.RecentDestination>} */
    recentDestinations_: {
      type: Array,
      notify: true,
      value: [],
    },

    /** @private {!print_preview_new.State} */
    state_: {
      type: Object,
      notify: true,
      value: {
        initialized: false,
        previewLoading: false,
        previewFailed: false,
        cloudPrintError: '',
        privetExtensionError: '',
        invalidSettings: false,
      },
    },
  },

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
    this.nativeLayer_ = print_preview.NativeLayer.getInstance();
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
        this.recentDestinations_);
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

  /**
   * @param {?string} savedSettingsStr The sticky settings from native layer
   * @private
   */
  updateFromStickySettings_(savedSettingsStr) {
    if (!savedSettingsStr) {
      this.set('state_.initialized', true);
      return;
    }

    let savedSettings;
    try {
      savedSettings = /** @type {print_preview_new.SerializedSettings} */ (
          JSON.parse(savedSettingsStr));
    } catch (e) {
      console.error('Unable to parse state ' + e);
      this.set('state_.initialized', true);
      return;  // use default values rather than updating.
    }
    if (savedSettings.version != 2) {
      this.set('state_.initialized', true);
      return;
    }

    let recentDestinations = savedSettings.recentDestinations || [];
    if (!Array.isArray(recentDestinations)) {
      recentDestinations = [recentDestinations];
    }
    this.set('recentDestinations_', recentDestinations);

    ['dpi', 'mediaSize', 'margins', 'color', 'headerFooter', 'layout',
     'collate', 'scaling', 'fitToPage', 'duplex', 'cssBackground']
        .forEach(settingName => {
          const setting = this.getSetting(settingName);
          if (savedSettings[setting.key] != undefined) {
            this.setSetting(settingName, savedSettings[setting.key]);
          }
        });
    this.set('state_.initialized', true);
  },
});
