// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-model',

  properties: {
    /**
     * Object containing current settings of Print Preview, for use by Polymer
     * controls.
     * @type {!print_preview_new.Settings}
     */
    settings: {
      type: Object,
      notify: true,
      value: {
        pages: {
          value: [1],
          valid: true,
          available: true,
          key: '',
        },
        copies: {
          value: '1',
          valid: true,
          available: true,
          key: '',
        },
        collate: {
          value: true,
          valid: true,
          available: true,
          key: 'isCollateEnabled',
        },
        layout: {
          value: false, /* portrait */
          valid: true,
          available: true,
          key: 'isLandscapeEnabled',
        },
        color: {
          value: true, /* color */
          valid: true,
          available: true,
          key: 'isColorEnabled',
        },
        mediaSize: {
          value: {
            width_microns: 215900,
            height_microns: 279400,
          },
          valid: true,
          available: true,
          key: 'mediaSize',
        },
        margins: {
          value: 0,
          valid: true,
          available: true,
          key: 'marginsType',
        },
        dpi: {
          value: {},
          valid: true,
          available: true,
          key: 'dpi',
        },
        fitToPage: {
          value: false,
          valid: true,
          available: true,
          key: 'isFitToPageEnabled',
        },
        scaling: {
          value: '100',
          valid: true,
          available: true,
          key: 'scaling',
        },
        duplex: {
          value: true,
          valid: true,
          available: true,
          key: 'isDuplexEnabled',
        },
        cssBackground: {
          value: false,
          valid: true,
          available: true,
          key: 'isCssBackgroundEnabled',
        },
        selectionOnly: {
          value: false,
          valid: true,
          available: true,
          key: '',
        },
        headerFooter: {
          value: true,
          valid: true,
          available: true,
          key: 'isHeaderFooterEnabled',
        },
        rasterize: {
          value: false,
          valid: true,
          available: true,
          key: '',
        },
        vendorItems: {
          value: {},
          valid: true,
          available: true,
          key: '',
        },
        // This does not represent a real setting value, and is used only to
        // expose the availability of the other options settings section.
        otherOptions: {
          value: null,
          valid: true,
          available: true,
          key: '',
        },
        serialization: {
          version: 2,
          recentDestinations: [],
        },
        initialized: false,
      },
    },

    /** @type {print_preview.Destination} */
    destination: {
      type: Object,
      notify: true,
    },

    /** @type {print_preview.DocumentInfo} */
    documentInfo: {
      type: Object,
      notify: true,
    },
  },

  observers:
      ['updateSettings_(' +
       'destination.id, destination.capabilities, ' +
       'documentInfo.isModifiable, documentInfo.hasCssMediaStyles,' +
       'documentInfo.hasSelection)'],

  /**
   * Updates the availability of the settings sections and values of dpi and
   *     media size settings.
   * @private
   */
  updateSettings_: function() {
    const caps = (!!this.destination && !!this.destination.capabilities) ?
        this.destination.capabilities.printer :
        null;
    this.updateSettingsAvailability_(caps);
    this.updateSettingsValues_(caps);
  },

  /**
   * @param {?print_preview.CddCapabilities} caps The printer capabilities.
   * @private
   */
  updateSettingsAvailability_: function(caps) {
    const isSaveToPdf = this.destination.id ==
        print_preview.Destination.GooglePromotedId.SAVE_AS_PDF;
    const knownSizeToSaveAsPdf = isSaveToPdf &&
        (!this.documentInfo.isModifiable ||
         this.documentInfo.hasCssMediaStyles);
    this.set('settings.copies.available', !!caps && !!(caps.copies));
    this.set('settings.collate.available', !!caps && !!(caps.collate));
    this.set('settings.layout.available', this.isLayoutAvailable_(caps));
    this.set('settings.color.available', this.destination.hasColorCapability);
    this.set('settings.margins.available', this.documentInfo.isModifiable);
    this.set(
        'settings.mediaSize.available',
        !!caps && !!caps.media_size && !knownSizeToSaveAsPdf);
    this.set(
        'settings.dpi.available',
        !!caps && !!caps.dpi && !!caps.dpi.option &&
            caps.dpi.option.length > 1);
    this.set(
        'settings.fitToPage.available',
        !this.documentInfo.isModifiable && !isSaveToPdf);
    this.set('settings.scaling.available', !knownSizeToSaveAsPdf);
    this.set('settings.duplex.available', !!caps && !!caps.duplex);
    this.set(
        'settings.cssBackground.available', this.documentInfo.isModifiable);
    this.set(
        'settings.selectionOnly.available',
        this.documentInfo.isModifiable && this.documentInfo.hasSelection);
    this.set('settings.headerFooter.available', this.documentInfo.isModifiable);
    this.set(
        'settings.rasterize.available',
        !this.documentInfo.isModifiable && !cr.isWindows && !cr.isMac);
    this.set(
        'settings.otherOptions.available',
        this.settings.duplex.available ||
            this.settings.cssBackground.available ||
            this.settings.selectionOnly.available ||
            this.settings.headerFooter.available ||
            this.settings.rasterize.available);
  },

  /**
   * @param {?print_preview.CddCapabilities} caps The printer capabilities.
   * @private
   */
  isLayoutAvailable_: function(caps) {
    if (!caps || !caps.page_orientation || !caps.page_orientation.option ||
        !this.documentInfo.isModifiable ||
        this.documentInfo.hasCssMediaStyles) {
      return false;
    }
    let hasAutoOrPortraitOption = false;
    let hasLandscapeOption = false;
    caps.page_orientation.option.forEach(option => {
      hasAutoOrPortraitOption = hasAutoOrPortraitOption ||
          option.type == 'AUTO' || option.type == 'PORTRAIT';
      hasLandscapeOption = hasLandscapeOption || option.type == 'LANDSCAPE';
    });
    return hasLandscapeOption && hasAutoOrPortraitOption;
  },

  /**
   * @param {?print_preview.CddCapabilities} caps The printer capabilities.
   * @private
   */
  updateSettingsValues_: function(caps) {
    if (this.settings.mediaSize.available) {
      for (const option of caps.media_size.option) {
        if (option.is_default) {
          this.set('settings.mediaSize.value', option);
          break;
        }
      }
    }

    if (this.settings.dpi.available) {
      for (const option of caps.dpi.option) {
        if (option.is_default) {
          this.set('settings.dpi.value', option);
          break;
        }
      }
    } else if (
        caps && caps.dpi && caps.dpi.option && caps.dpi.option.length > 0) {
      this.set('settings.dpi.value', caps.dpi.option[0]);
    }
  }
});
