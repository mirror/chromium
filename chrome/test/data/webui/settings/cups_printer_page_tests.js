// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @implements {settings.CupsPrintersBrowserProxy} */
class TestCupsPrintersBrowserProxy extends TestBrowserProxy {
  constructor() {
    super([
      'addDiscoveredPrinter',
      'getCupsPrintersList',
      'getCupsPrinterManufacturersList',
      'getCupsPrinterModelsList',
      'getPrinterInfo',
      'startDiscoveringPrinters',
      'stopDiscoveringPrinters',
      'cancelPrinterSetUp',
    ]);

    this.printerList = [];
    this.manufacturers = [];
    this.models = [];
    this.printerInfo = {};
  }

  /** @override */
  addDiscoveredPrinter(printerId) {
    this.methodCalled('addDiscoveredPrinter', printerId);
  }

  /** @override */
  getCupsPrintersList() {
    this.methodCalled('getCupsPrintersList');
    return Promise.resolve(this.printerList);
  }

  /** @override */
  getCupsPrinterManufacturersList() {
    this.methodCalled('getCupsPrinterManufacturersList');
    return Promise.resolve(this.manufacturers);
  }

  /** @override */
  getCupsPrinterModelsList(manufacturer) {
    this.methodCalled('getCupsPrinterModelsList', manufacturer);
    return Promise.resolve(this.models);
  }

  /** @override */
  getPrinterInfo(newPrinter) {
    this.methodCalled('getPrinterInfo', newPrinter);
    return Promise.resolve(this.printerInfo);
  }

  /** @override */
  startDiscoveringPrinters() {
    this.methodCalled('startDiscoveringPrinters');
  }

  /** @override */
  stopDiscoveringPrinters() {
    this.methodCalled('stopDiscoveringPrinters');
  }

  /** @override */
  cancelPrinterSetUp(newPrinter) {
    this.methodCalled('cancelPrinterSetUp', newPrinter);
  }
}

suite('CupsAddPrinterDialogTests', function() {
  function fillAddManuallyDialog(addDialog, nameValue, addressValue) {
    var name = addDialog.$$('#printerNameInput');
    var address = addDialog.$$('#printerAddressInput');

    assertTrue(!!name);
    name.value = nameValue;

    assertTrue(!!address);
    address.value = addressValue;
  }

  function clickAddButton(dialog) {
    assertTrue(!!dialog, 'Dialog is null for add');
    var addButton = dialog.$$('.action-button');
    assertTrue(!!addButton, 'Button is null');
    MockInteractions.tap(addButton);
  }

  function clickCancelButton(dialog) {
    assertTrue(!!dialog, 'Dialog is null for cancel');
    var cancelButton = dialog.$$('.cancel-button');
    assertTrue(!!cancelButton, 'Button is null');
    MockInteractions.tap(cancelButton);
  }

  var page = null;
  var dialog = null;

  /** @type {?settings.TestCupsPrintersBrowserProxy} */
  var cupsPrintersBrowserProxy = null;

  setup(function() {
    cupsPrintersBrowserProxy = new TestCupsPrintersBrowserProxy();
    settings.CupsPrintersBrowserProxyImpl.instance_ = cupsPrintersBrowserProxy;

    PolymerTest.clearBody();
    page = document.createElement('settings-cups-printers');
    document.body.appendChild(page);
    assertTrue(!!page);
    dialog = page.$$('settings-cups-add-printer-dialog');
    assertTrue(!!dialog);

    dialog.open();
    Polymer.dom.flush();
  });

  teardown(function() {
    cupsPrintersBrowserProxy.reset();
    page.remove();
    dialog = null;
    page = null;
  });

  /**
   * Test that the discovery dialog is showing when a user initially asks
   * to add a printer.
   */
  test('DiscoveryShowing', function() {
    return PolymerTest.flushTasks().then(function() {
      // Discovery is showing.
      assertTrue(dialog.showDiscoveryDialog_);
      assertTrue(!!dialog.$$('add-printer-discovery-dialog'));

      // All other components are hidden.
      assertFalse(dialog.showManufacturerDialog_);
      assertFalse(!!dialog.$$('add-printer-manufacturer-model-dialog'));
      assertFalse(dialog.showConfiguringDialog_);
      assertFalse(!!dialog.$$('add-printer-configuring-dialog'));
      assertFalse(dialog.showManuallyAddDialog_);
      assertFalse(!!dialog.$$('add-printer-manually-dialog'));
    });
  });

  /**
   * Test that canAddPrinter_() in the add printer manually dialog correctly
   * verifies addresses.
   */
  test('CanAddPrinter', function() {
    // Starts in discovery dialog, select add manually button.
    var discoveryDialog = dialog.$$('add-printer-discovery-dialog');
    assertTrue(!!discoveryDialog);
    MockInteractions.tap(discoveryDialog.$$('.secondary-button'));
    Polymer.dom.flush();

    var addDialog = dialog.$$('add-printer-manually-dialog');
    assertTrue(!!addDialog);

    var name = addDialog.$$('#printerNameInput');
    var address = addDialog.$$('#printerAddressInput');

    // Testing a valid ipv4 address.
    fillAddManuallyDialog(addDialog, 'Test printer', '127.0.0.1');
    assertTrue(addDialog.canAddPrinter_(name.value, address.value));

    // Testing valid ipv6 addresses.
    fillAddManuallyDialog(addDialog, 'Test printer', '1:2:a3:ff4:5:6:7:8');
    assertTrue(addDialog.canAddPrinter_(name.value, address.value));

    fillAddManuallyDialog(addDialog, 'Test printer', '1:::22');
    assertTrue(addDialog.canAddPrinter_(name.value, address.value));

    fillAddManuallyDialog(addDialog, 'Test printer', '::255');
    assertTrue(addDialog.canAddPrinter_(name.value, address.value));

    // Testing invalid ipv6 addresses.
    fillAddManuallyDialog(addDialog, 'Test printer', '1:2:3:zs:2');
    assertFalse(addDialog.canAddPrinter_(name.value, address.value));

    // This is an invalid address, but it will be accepted by our regex. There
    // is only supposed to be at most one instance of '::' in a valid ipv6
    // address.
    fillAddManuallyDialog(addDialog, 'Test printer', '1::2::3');
    assertTrue(addDialog.canAddPrinter_(name.value, address.value));

    // Testing valid hostnames
    fillAddManuallyDialog(addDialog, 'Test printer', 'hello-world.com');
    assertTrue(addDialog.canAddPrinter_(name.value, address.value));

    fillAddManuallyDialog(addDialog, 'Test printer', 'hello.world.com:12345');
    assertTrue(addDialog.canAddPrinter_(name.value, address.value));

    // Testing invalid hostname.
    fillAddManuallyDialog(addDialog, 'Test printer', 'helloworld!123.com');
    assertFalse(addDialog.canAddPrinter_(name.value, address.value));
  });

  /**
   * Test that clicking on Add opens the model select page.
   */
  test('ValidAddOpensModelSelection', function() {
    // Starts in discovery dialog, select add manually button.
    var discoveryDialog = dialog.$$('add-printer-discovery-dialog');
    assertTrue(!!discoveryDialog);
    MockInteractions.tap(discoveryDialog.$$('.secondary-button'));
    Polymer.dom.flush();

    // Now we should be in the manually add dialog.
    var addDialog = dialog.$$('add-printer-manually-dialog');
    assertTrue(!!addDialog);
    fillAddManuallyDialog(addDialog, 'Test printer', '127.0.0.1');

    MockInteractions.tap(addDialog.$$('.action-button'));
    Polymer.dom.flush();
    // Configure is shown until getPrinterInfo is rejected.
    assertTrue(!!dialog.$$('add-printer-configuring-dialog'));

    // Upon rejection, show model.
    return cupsPrintersBrowserProxy.
        whenCalled('getCupsPrinterManufacturersList').
        then(function() {
          return PolymerTest.flushTasks();
        }).
        then(function() {
          // Showing model selection.
          assertFalse(!!dialog.$$('add-printer-configuring-dialog'));
          assertTrue(!!dialog.$$('add-printer-manufacturer-model-dialog'));

          assertTrue(dialog.showManufacturerDialog_);
          assertFalse(dialog.showConfiguringDialog_);
          assertFalse(dialog.showManuallyAddDialog_);
          assertFalse(dialog.showDiscoveryDialog_);
        });
  });

  /**
   * Test that getModels isn't called with a blank query.
   */
  test('NoBlankQueries', function() {
    var discoveryDialog = dialog.$$('add-printer-discovery-dialog');
    assertTrue(!!discoveryDialog);
    MockInteractions.tap(discoveryDialog.$$('.secondary-button'));
    Polymer.dom.flush();

    var addDialog = dialog.$$('add-printer-manually-dialog');
    assertTrue(!!addDialog);
    fillAddManuallyDialog(addDialog, 'Test printer', '127.0.0.1');

    // Verify that getCupsPrinterModelList is not called.
    cupsPrintersBrowserProxy.whenCalled('getCupsPrinterModelsList')
        .then(function(manufacturer) {
          assertNotReached(
              'No manufacturer was selected.  Unexpected model request.');
        });

    cupsPrintersBrowserProxy.manufacturers =
        ['ManufacturerA', 'ManufacturerB', 'Chromites'];
    MockInteractions.tap(addDialog.$$('.action-button'));
    Polymer.dom.flush();

    return cupsPrintersBrowserProxy
        .whenCalled('getCupsPrinterManufacturersList')
        .then(function() {
          let modelDialog = dialog.$$('add-printer-manufacturer-model-dialog');
          assertTrue(!!modelDialog);
          // Manufacturer dialog has been rendered and the model list was not
          // requested.  We're done.
        });
  });

  /**
   * Test that dialog cancellation is logged from the manufacturer screen for
   * IPP printers.
   */
  test('LogDialogCancelledIpp', function() {
    var makeAndModel = 'Printer Make And Model';
    // Start on add manually.
    dialog.fire('open-manually-add-printer-dialog');
    Polymer.dom.flush();

    // Populate the printer object.
    dialog.newPrinter = {
      ppdManufacturer: '',
      ppdModel: '',
      printerAddress: '192.168.1.13',
      printerAutoconf: false,
      printerDescription: '',
      printerId: '',
      printerManufacturer: '',
      printerModel: '',
      printerMakeAndModel: '',
      printerName: 'Test Printer',
      printerPPDPath: '',
      printerProtocol: 'ipps',
      printerQueue: 'moreinfohere',
      printerStatus: '',
    };

    // Seed the getPrinterInfo response.  We detect the make and model but it is
    // not an autoconf printer.
    cupsPrintersBrowserProxy.printerInfo = {
      autoconf: false,
      manufacturer: 'MFG',
      model: 'MDL',
      makeAndModel: makeAndModel,
    };

    // Press the add button to advance dialog.
    var addDialog = dialog.$$('add-printer-manually-dialog');
    assertTrue(!!addDialog);
    clickAddButton(addDialog);

    // Click cancel on the manufacturer dialog when it shows up then verify
    // cancel was called with the appropriate parameters.
    return cupsPrintersBrowserProxy
        .whenCalled('getCupsPrinterManufacturersList')
        .then(function() {
          Polymer.dom.flush();
          // Cancel setup with the cancel button.
          clickCancelButton(dialog.$$('add-printer-manufacturer-model-dialog'));
          return cupsPrintersBrowserProxy.whenCalled('cancelPrinterSetUp');
        })
        .then(function(printer) {
          assertTrue(!!printer, 'New printer is null');
          assertEquals(makeAndModel, printer.printerMakeAndModel);
        });
  });

  /**
   * Test that dialog cancellation is logged from the manufacturer screen for
   * USB printers.
   */
  test('LogDialogCancelledUSB', function() {
    var vendorId = 0x1234;
    var modelId = 0xDEAD;
    var manufacturer = 'PrinterMFG';
    var model = 'Printy Printerson';

    var usbInfo = {
      usbVendorId: vendorId,
      usbProductId: modelId,
      usbVendorName: manufacturer,
      usbProductName: model,
    };

    var expectedPrinter = 'PICK_ME!';

    var newPrinter = {
      ppdManufacturer: '',
      ppdModel: '',
      printerAddress: 'EEAADDAA',
      printerAutoconf: false,
      printerDescription: '',
      printerId: expectedPrinter,
      printerManufacturer: '',
      printerModel: '',
      printerMakeAndModel: '',
      printerName: '',
      printerPPDPath: '',
      printerProtocol: 'usb',
      printerQueue: 'moreinfohere',
      printerStatus: '',
      printerUsbInfo: usbInfo,
    };

    dialog.fire('open-discovery-printers-dialog');

    return cupsPrintersBrowserProxy.whenCalled('startDiscoveringPrinters')
        .then(function() {
          // Select the printer.
          // TODO(skau): Figure out how to select in a dom-repeat.
          let discoveryDialog = dialog.$$('add-printer-discovery-dialog');
          assertTrue(!!discoveryDialog, 'Cannot find discovery dialog');
          discoveryDialog.selectedPrinter = newPrinter;
          // Run printer setup.
          clickAddButton(discoveryDialog);
          return cupsPrintersBrowserProxy.whenCalled('addDiscoveredPrinter');
        })
        .then(function(printerId) {
          assertEquals(expectedPrinter, printerId);

          cr.webUIListenerCallback(
              'on-manually-add-discovered-printer', newPrinter);
          return cupsPrintersBrowserProxy.whenCalled(
              'getCupsPrinterManufacturersList');
        })
        .then(function() {
          // Cancel setup with the cancel button.
          clickCancelButton(dialog.$$('add-printer-manufacturer-model-dialog'));
          return cupsPrintersBrowserProxy.whenCalled('cancelPrinterSetUp');
        })
        .then(function(printer) {
          assertEquals(expectedPrinter, printer.printerId);
          assertDeepEquals(usbInfo, printer.printerUsbInfo);
        });
  });
});
