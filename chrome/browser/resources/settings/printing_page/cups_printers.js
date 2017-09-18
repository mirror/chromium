// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'settings-cups-printers' is a component for showing CUPS
 * Printer settings subpage (chrome://settings/cupsPrinters). It is used to
 * set up legacy & non-CloudPrint printers on ChromeOS by leveraging CUPS (the
 * unix printing system) and the many open source drivers built for CUPS.
 */
// TODO(xdai): Rename it to 'settings-cups-printers-page'.
Polymer({
  is: 'settings-cups-printers',

  behaviors: [WebUIListenerBehavior],

  properties: {
    /** @type {!Array<!CupsPrinterInfo>} */
    printers: {
      type: Array,
      notify: true,
    },

    /** @type {?CupsPrinterInfo} */
    activePrinter: {
      type: Object,
      notify: true,
    },

    searchTerm: {
      type: String,
    },

    /** @private */
    canAddPrinter_: Boolean,

    /** @private */
    showCupsEditPrinterDialog_: Boolean,
  },

  listeners: {
    'edit-cups-printer-details': 'onShowCupsEditPrinterDialog_',
  },

  /**
   * @type {function()}
   * @private
   */
  networksChangedListener_: function() {},

  /** @override */
  ready: function() {
    this.updateCupsPrintersList_();
    this.refreshNetworks_();
  },

  /** @override */
  attached: function() {
    this.addWebUIListener('on-add-cups-printer', this.onAddPrinter_.bind(this));
    this.networksChangedListener_ = this.refreshNetworks_.bind(this);
    chrome.networkingPrivate.onNetworksChanged.addListener(
        this.networksChangedListener_);
  },

  /** @override */
  detached: function() {
    chrome.networkingPrivate.onNetworksChanged.removeListener(
        this.networksChangedListener_);
  },

  /**
   * Callback function when networks change.
   * @private
   */
  refreshNetworks_: function() {
    chrome.networkingPrivate.getNetworks(
        {
          'networkType': chrome.networkingPrivate.NetworkType.ALL,
          'configured': true
        },
        this.onNetworksReceived_.bind(this));
  },

  /**
   * Callback function when configured networks are received.
   * @param {!Array<!chrome.networkingPrivate.NetworkStateProperties>} states
   *     A list of network state information for each network.
   * @private
   */
  onNetworksReceived_: function(states) {
    this.canAddPrinter_ = states.some(function(entry) {
      return entry.hasOwnProperty('ConnectionState') &&
          entry.ConnectionState == 'Connected';
    });
  },

  /**
   * @param {PrinterSetupResult} result_code
   * @param {string} printerName
   * @private
   */
  onAddPrinter_: function(result_code, printerName) {
    if (result_code == PrinterSetupResult.kSuccess) {
      this.updateCupsPrintersList_();
      var message = this.$.addPrinterDoneMessage;
      message.textContent =
          loadTimeData.getStringF('printerAddedSuccessfulMessage', printerName);
    } else {
      var message = this.$.addPrinterErrorMessage;
      var messageText = this.$.addPrinterFailedMessage;
      switch (result_code) {
        case PrinterSetupResult.kFatalError:
          messageText.textContent =
              loadTimeData.getString('printerAddedFatalErrorMessage');
          break;
        case PrinterSetupResult.kPrinterUnreachable:
          messageText.textContent =
              loadTimeData.getString('printerAddedPrinterUnreachableMessage');
          break;
        case PrinterSetupResult.kDbusError:
          // PrinterSetupResult::kDbusError
          messageText.textContent =
              loadTimeData.getString('printerAddedDbusErrorMessage');
        case PrinterSetupResult.kPpdTooLarge:
          // PrinterSetupResult::kPpdTooLarge
          messageText.textContent =
              loadTimeData.getString('printerAddedPpdTooLargeMessage');
          break;
        case PrinterSetupResult.kInvalidPpd:
          messageText.textContent =
              loadTimeData.getString('printerAddedInvalidPpdMessage');
          break;
        case PrinterSetupResult.kPpdNotFound:
          messageText.textContent =
              loadTimeData.getString('printerAddedPpdNotFoundMessage');
          break;
        case PrinterSetupResult.kPpdUnretrievable:
          messageText.textContent =
              loadTimeData.getString('printerAddedPpdUnretrievableMessage');
      }
    }

    message.hidden = false;
    window.setTimeout(function() {
      message.hidden = true;
    }, 3000);
  },

  /** @private */
  updateCupsPrintersList_: function() {
    settings.CupsPrintersBrowserProxyImpl.getInstance()
        .getCupsPrintersList()
        .then(this.printersChanged_.bind(this));
  },

  /**
   * @param {!CupsPrintersList} cupsPrintersList
   * @private
   */
  printersChanged_: function(cupsPrintersList) {
    this.printers = cupsPrintersList.printerList;
  },

  /** @private */
  onAddPrinterTap_: function() {
    this.$.addPrinterDialog.open();
    this.$.addPrinterErrorMessage.hidden = true;
  },

  /** @private */
  onAddPrinterDialogClose_: function() {
    cr.ui.focusWithoutInk(assert(this.$$('#addPrinter')));
  },

  /** @private */
  onShowCupsEditPrinterDialog_: function() {
    this.showCupsEditPrinterDialog_ = true;
    this.async(function() {
      var dialog = this.$$('settings-cups-edit-printer-dialog');
      dialog.addEventListener('close', function() {
        this.showCupsEditPrinterDialog_ = false;
      }.bind(this));
    });
  },

});
