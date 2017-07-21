// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-cups-edit-printer-dialog' is a dialog to edit the
 * existing printer's information and re-configure it.
 */

Polymer({
  is: 'settings-cups-edit-printer-dialog',

  properties: {
    /** @type {!CupsPrinterInfo} */
    activePrinter: {
      type: Object,
      notify: true,
    },

    /** @type {?Array<string>} */
    manufacturerList: {
      type: Array,
    },

    /** @type {?Array<string>} */
    modelList: {
      type: Array,
    },
  },

  observers: [
    'selectedManufacturerChanged_(activePrinter.ppdManufacturer)',
  ],

  /** @override */
  ready: function() {
    settings.CupsPrintersBrowserProxyImpl.getInstance()
        .getCupsPrinterManufacturersList()
        .then(this.manufacturerListChanged_.bind(this));
  },

  /**
   * @param {!ManufacturersInfo} manufacturersInfo
   * @private
   */
  manufacturerListChanged_: function(manufacturersInfo) {
    if (!manufacturersInfo.success)
      return;
    this.manufacturerList = manufacturersInfo.manufacturers;
    if (this.activePrinter.ppdManufacturer.length != 0) {
      settings.CupsPrintersBrowserProxyImpl.getInstance()
          .getCupsPrinterModelsList(this.activePrinter.ppdManufacturer)
          .then(this.modelListChanged_.bind(this));
    }
  },

  /**
   * @param {string} manufacturer The manufacturer for which we are retrieving
   *     models.
   * @private
   */
  selectedManufacturerChanged_: function(manufacturer) {
    // Reset model if manufacturer is changed.
    if (!this.activePrinter.ppdModel.startsWith(manufacturer) ||
        manufacturer.length == 0) {
      this.set('activePrinter.ppdModel', '');
      this.set('activePrinter.printerMakeAndModel', '');
      settings.CupsPrintersBrowserProxyImpl.getInstance()
          .getCupsPrinterModelsList(manufacturer)
          .then(this.modelListChanged_.bind(this));
    }
  },

  /**
   * @param {!ModelsInfo} modelsInfo
   * @private
   */
  modelListChanged_: function(modelsInfo) {
    if (modelsInfo.success)
      this.modelList = modelsInfo.models;
  },

  /**
   * @param {!Event} event
   * @private
   */
  onProtocolChange_: function(event) {
    this.set('activePrinter.printerProtocol', event.target.value);
  },

  /**
   * @param {!CupsPrinterInfo} printer
   * @return {string} The printer's URI that displays in the UI
   * @private
   */
  getPrinterURI_: function(printer) {
    if (!printer) {
      return '';
    } else if (
        printer.printerProtocol && printer.printerAddress &&
        printer.printerQueue) {
      return printer.printerProtocol + '://' + printer.printerAddress + '/' +
          printer.printerQueue;
    } else if (printer.printerProtocol && printer.printerAddress) {
      return printer.printerProtocol + '://' + printer.printerAddress;
    } else {
      return '';
    }
  },

  /** @private */
  onBrowseFile_: function() {
    settings.CupsPrintersBrowserProxyImpl.getInstance()
        .getCupsPrinterPPDPath()
        .then(this.printerPPDPathChanged_.bind(this));
  },

  /**
   * @param {string} path
   * @private
   */
  printerPPDPathChanged_: function(path) {
    this.set('activePrinter.printerPPDPath', path);
    this.$$('#browseFileInput').value = this.getBaseName_(path);
  },

  /**
   * @param {string} path The full path of the file
   * @return {string} The base name of the file
   * @private
   */
  getBaseName_: function(path) {
    return path.substring(path.lastIndexOf('/') + 1);
  },

  /** @private */
  onCancelTap_: function() {
    this.$$('add-printer-dialog').close();
  },

  /** @private */
  onSaveTap_: function() {
    settings.CupsPrintersBrowserProxyImpl.getInstance().addCupsPrinter(
        this.activePrinter);
    this.$$('add-printer-dialog').close();
  },
});
