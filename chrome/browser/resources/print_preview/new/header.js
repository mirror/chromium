// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

Polymer({
  is: 'print-preview-header',

  behaviors: [SettingsBehavior, StateBehavior],

  properties: {
    /** @type {!print_preview.Destination} */
    destination: Object,

    /** @private {boolean} */
    printButtonEnabled_: {
      type: Boolean,
      value: false,
    },

    /** @private {?string} */
    summary_: {
      type: String,
      notify: true,
      value: null,
    },

    /** @private {?string} */
    summaryLabel_: {
      type: String,
      notify: true,
      value: null,
    },

    errorMessage: String,
  },

  observers:
      ['onSettingChanged_(settings.copies.value, settings.duplex.value)'],

  /** @private */
  onPrintTap_: function() {
    if (this.state == print_preview_new.State.PREVIEW_LOADED)
      this.transitTo(print_preview_new.State.PRINTING);
    else
      this.transitTo(print_preview_new.State.HIDDEN);
  },

  /** @private */
  onCancelTap_: function() {
    this.transitTo(print_preview_new.State.CANCELLED);
  },

  /**
   * @return {?string}
   * @private
   */
  currentSummary_: function() {
    return this.summary_;
  },

  /**
   * @return {?string}
   * @private
   */
  currentLabel_: function() {
    return this.summaryLabel_;
  },


  /**
   * @return {boolean}
   * @private
   */
  isPdfOrDrive_: function() {
    return this.destination &&
        (this.destination.id ==
             print_preview.Destination.GooglePromotedId.SAVE_AS_PDF ||
         this.destination.id ==
             print_preview.Destination.GooglePromotedId.DOCS);
  },

  /**
   * @return {string}
   * @private
   */
  getPrintButton_: function() {
    return loadTimeData.getString(
        this.isPdfOrDrive_() ? 'saveButton' : 'printButton');
  },

  /** @private */
  onSettingChanged_: function() {
    if (!this.settings || !this.settings.pages)
      return;
    this.summary_ = this.getSummary_();
    this.summaryLabel_ = this.getSummaryLabel_();
  },

  /**
   * @return {{numPages: number,
   *           numSheets: number,
   *           pagesLabel: string,
   *           summaryLabel: string}}
   * @private
   */
  computeLabelInfo_: function() {
    const saveToPdfOrDrive = this.isPdfOrDrive_();
    let numPages = this.getSetting('pages').value.length;
    let numSheets = numPages;
    if (!saveToPdfOrDrive && this.getSetting('duplex').value) {
      numSheets = Math.ceil(numPages / 2);
    }

    const copies = /** @type {number} */ (this.getSetting('copies').value);
    numSheets *= copies;
    numPages *= copies;

    const pagesLabel = loadTimeData.getString('printPreviewPageLabelPlural');
    let summaryLabel;
    if (numSheets > 1) {
      summaryLabel = saveToPdfOrDrive ?
          pagesLabel :
          loadTimeData.getString('printPreviewSheetsLabelPlural');
    } else {
      summaryLabel = loadTimeData.getString(
          saveToPdfOrDrive ? 'printPreviewPageLabelSingular' :
                             'printPreviewSheetsLabelSingular');
    }
    return {
      numPages: numPages,
      numSheets: numSheets,
      pagesLabel: pagesLabel,
      summaryLabel: summaryLabel
    };
  },

  onStateChanged: function(error) {
    switch (this.state) {
      case (print_preview_new.State.PRINTING):
        this.printButtonEnabled_ = false;
        this.summary_ = loadTimeData.getString(
            this.isPdfOrDrive_() ? 'saving' : 'printing');
        this.summaryLabel_ = loadTimeData.getString(
            this.isPdfOrDrive_() ? 'saving' : 'printing');
        break;
      case (print_preview_new.State.READY):
        this.printButtonEnabled_ = this.destination.isLocal &&
            !this.destination.isExtension && !this.destination.isPrivet &&
            this.destination.id !=
                print_preview.Destination.GooglePromotedId.SAVE_AS_PDF;
        this.summary_ = this.getSummary_();
        this.summaryLabel_ = this.getSummaryLabel_();
        break;
      case (print_preview_new.State.PREVIEW_LOADED):
        this.printButtonEnabled_ = true;
        this.summary_ = this.getSummary_();
        this.summaryLabel_ = this.getSummaryLabel_();
        break;
      case (print_preview_new.State.FATAL_ERROR):
        this.printButtonEnabled_ = false;
        this.summary_ = this.errorMessage;
        this.summaryLabel_ = this.errorMessage;
        break;
      default:
        this.summary_ = null;
        this.summaryLabel_ = null;
        this.printButtonEnabled_ = false;
        break;
    }
  },

  /**
   * @return {string}
   * @private
   */
  getSummary_: function() {
    let html = null;
    const labelInfo = this.computeLabelInfo_();
    if (labelInfo.numPages != labelInfo.numSheets) {
      html = loadTimeData.getStringF(
          'printPreviewSummaryFormatLong',
          '<b>' + labelInfo.numSheets.toLocaleString() + '</b>',
          '<b>' + labelInfo.summaryLabel + '</b>',
          labelInfo.numPages.toLocaleString(), labelInfo.pagesLabel);
    } else {
      html = loadTimeData.getStringF(
          'printPreviewSummaryFormatShort',
          '<b>' + labelInfo.numSheets.toLocaleString() + '</b>',
          '<b>' + labelInfo.summaryLabel + '</b>');
    }

    // Removing extra spaces from within the string.
    html = html.replace(/\s{2,}/g, ' ');
    return html;
  },

  /**
   * @return {string}
   * @private
   */
  getSummaryLabel_: function() {
    const labelInfo = this.computeLabelInfo_();
    if (labelInfo.numPages != labelInfo.numSheets) {
      return loadTimeData.getStringF(
          'printPreviewSummaryFormatLong', labelInfo.numSheets.toLocaleString(),
          labelInfo.summaryLabel, labelInfo.numPages.toLocaleString(),
          labelInfo.pagesLabel);
    }
    return loadTimeData.getStringF(
        'printPreviewSummaryFormatShort', labelInfo.numSheets.toLocaleString(),
        labelInfo.summaryLabel);
  }
});
