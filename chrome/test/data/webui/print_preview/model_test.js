// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('model_test', function() {
  /** @enum {string} */
  const TestNames = {
    SetStickySettings: 'set sticky settings',
  };

  const suiteName = 'ModelTest';
  suite(suiteName, function() {
    let model = null;

    /** @override */
    setup(function() {
      model = document.createElement('print-preview-model');
      document.body.appendChild(model);
    });

    /**
     * Tests state restoration with all boolean settings set to true, scaling =
     * 90, dpi = 100, custom square paper, and custom margins.
     */
    test(assert(TestNames.SetStickySettings), function() {
      // Default state of the model.
      const stickySettingsDefault = {
        version: 2,
        recentDestinations: [],
        dpi: {},
        mediaSize: {width_microns: 215900,
                    height_microns: 279400},
        customMargins: {top: 74, right: 74, bottom: 74, left: 74},
        vendorOptions: {},
        marginsType: 0,  /* default */
        scaling: '100',
        isHeaderFooterEnabled: true,
        isCssBackgroundEnabled: false,
        isFitToPageEnabled: true,
        isCollateEnabled: true,
        isDuplexEnabled: true,
        isLandscapeEnabled: false,
        isColorEnabled: true,
      };

      // Non-default state
      const stickySettingsChange = {
        version: 2,
        recentDestinations: [],
        dpi: {horizontal_dpi: 1000, vertical_dpi: 500},
        mediaSize: {width_microns: 43180,
                    height_microns: 21590},
        customMargins: {top: 74, right: 74, bottom: 74, left: 74},
        vendorOptions: {},
        marginsType: 2,  /* none */
        scaling: '85',
        isHeaderFooterEnabled: false,
        isCssBackgroundEnabled: true,
        isFitToPageEnabled: false,
        isCollateEnabled: false,
        isDuplexEnabled: false,
        isLandscapeEnabled: true,
        isColorEnabled: false,
      };

      const validateAndReset = function(field, settingsStr) {
        let settings = JSON.parse(settingsStr);
        for (let settingName in settings) {
          if (settings.hasOwnProperty(settingName)) {
            if (settingName == field) {
              expectEquals(stickySettingsChange[settingName],
                           settings[settingName]);
            } else {
              expectEquals(stickySettingsDefault[settingName],
                           settings[settingName]);
            }
          }
        }
      };

      [['margins', 'marginsType'],
       ['color', 'isColorEnabled'], ['headerFooter', 'isHeaderFooterEnabled'],
       ['layout', 'isLandscapeEnabled'], ['collate', 'isCollateEnabled'],
       ['fitToPage', 'isFitToPageEnabled'],
       ['cssBackground', 'isCssBackgroundEnabled'], ['scaling', 'scaling'],
      ].forEach(keys => {
        let promise = test_util.eventToPromise('save-sticky-settings', model);
        model.set(`settings.${keys[1]}.value`, stickySettingsChange[keys[2]]);
        return promise.then(function(settingsStr) {
          validateChange(keys[2], settingsStr);
          model.set(`settings.${keys[1]}.value`,
                    stickySettingsDefault[keys[2]]);
        });
      });
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});
