// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('restore_state_test', function() {
  /** @enum {string} */
  const TestNames = {
    Restore: 'restore',
  };

  const suiteName = 'RestoreStateTest';
  suite(suiteName, function() {
    let page = null;
    let nativeLayer = null;

    const initialSettings = {
      isInKioskAutoPrintMode: false,
      isInAppKioskMode: false,
      thousandsDelimeter: ',',
      decimalDelimeter: '.',
      unitType: 1,
      previewModifiable: true,
      documentTitle: 'title',
      documentHasSelection: true,
      shouldPrintSelectionOnly: false,
      printerName: 'FooDevice',
      serializedAppStateStr: null,
      serializedDefaultDestinationSelectionRulesStr: null
    };

    /** @override */
    setup(function() {
      nativeLayer = new print_preview.NativeLayerStub();
      print_preview.NativeLayer.setInstance(nativeLayer);
      PolymerTest.clearBody();
    });

    test(assert(TestNames.Restore), function() {
      const stickySettings = JSON.stringify({
        version: 2,
        recentDestinations: [],
        dpi: {horizontal_dpi: 100, vertical_dpi: 100},
        mediaSize: {name: 'CUSTOM_SQUARE',
                    width_microns: 215900,
                    height_microns: 215900,
                    custom_display_name: 'CUSTOM_SQUARE'},
        customMargins: {top: 74, right: 74, bottom: 74, left: 74},
        vendorOptions: {},
        marginsType: 3,  /* custom */
        scaling: '90',
        isHeaderFooterEnabled: true,
        isCssBackgroundEnabled: true,
        isFitToPageEnabled: true,
        isCollateEnabled: true,
        isDuplexEnabled: true,
        isLandscapeEnabled: true,
        isColorEnabled: true,
      });
      initialSettings.serializedAppStateStr = stickySettings;

      nativeLayer.setInitialSettings(initialSettings);
      page = document.createElement('print-preview-app');
      document.body.appendChild(page);
      return nativeLayer.whenCalled('getInitialSettings').then(function() {
        assertEquals(page.settings.dpi.value.horizontal_dpi, 100);
        assertEquals(page.settings.mediaSize.value.name, 'CUSTOM_SQUARE');
        assertEquals(page.settings.margins.value, 3);
        assertEquals(page.settings.scaling.value, '90');
        assertTrue(page.settings.headerFooter.value);
        assertTrue(page.settings.cssBackground.value);
        assertTrue(page.settings.fitToPage.value);
        assertTrue(page.settings.collate.value);
        assertTrue(page.settings.duplex.value);
        assertTrue(page.settings.layout.value);
        assertTrue(page.settings.color.value);
      });
    });
  });

  return {
    suiteName: suiteName,
    TestNames: TestNames,
  };
});

