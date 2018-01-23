// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'passwords-export-dialog' is the dialog that allows exporting
 * passwords.
 */

(function() {
'use strict';

/**
 * The states of the export passwords dialog.
 * @enum {string}
 */
const States = {
  START: 'START',
  IN_PROGRESS: 'IN_PROGRESS',
  ERROR: 'ERROR',
};

const ProgressStatus = chrome.passwordsPrivate.ExportProgressStatus;

/**
 * The amount of time (ms) between the start of the export and the moment we
 * start showing the progress bar.
 * @type {number}
 */
const progressBarDelay = 100;

/**
 * The minimum amount of time (ms) that the progress bar will be visible.
 * @type {number}
 */
const progressBarBlock = 2000;

Polymer({
  is: 'passwords-export-dialog',

  behaviors: [
    I18nBehavior,
  ],

  properties: {
    /**
     * The error that occurred while exporting.
     */
    exportErrorMessage: {
      type: String,
      value: '',
    },
  },

  listeners: {
    'cancel': 'close',
  },

  /**
   * The interface for callbacks to the browser.
   * Defined in passwords_section.js
   * @type {PasswordManager}
   * @private
   */
  passwordManager_: null,

  /**
   * @type {function(!PasswordManager.PasswordExportProgress):void}
   * @private
   */
  onPasswordsFileExportProgressListener_: null,

  /**
   * The task that will display the progress bar, if the export doesn't finish
   * quickly. This is null, unless the task is currently scheduled.
   * @type {?number}
   * @private
   */
  progressTaskToken_: null,

  /**
   * The task that will display the completion of the export, if any. We display
   * the progress bar for at least 2000ms, therefore, if export finishes
   * earlier, we cache the result in |delayedProgress_| and this task will
   * consume it. This is null, unless the task is currently scheduled.
   * @type {?number}
   * @private
   */
  delayedCompletionToken_: null,

  /**
   * We display the progress bar for at least 2000ms. If progress is achieved
   * earlier, we store the update here and consume it later.
   * @type {?PasswordManager.PasswordExportProgress}
   * @private
   */
  delayedProgress_: null,

  /**
   * Displays the progress bar and suspends further UI updates for
   * |progressBarBlock|ms.
   * @private
   */
  progressTask() {
    this.progressTaskToken_ = null;
    this.switchToDialog(States.IN_PROGRESS);

    this.delayedCompletionToken_ = setTimeout(() => {
      this.delayedCompletionTask();
    }, progressBarBlock);
  },

  /**
   * Unblocks progress after showing the progress bar for |progressBarBlock|ms
   * and processes any progress that was delayed.
   * @private
   */
  delayedCompletionTask() {
    this.delayedCompletionToken_ = null;
    if (this.delayedProgress_) {
      this.processProgress(this.delayedProgress_);
      this.delayedProgress_ = null;
    }
  },

  /** @override */
  attached: function() {
    this.passwordManager_ = PasswordManagerImpl.getInstance();

    this.switchToDialog(States.START);

    this.onPasswordsFileExportProgressListener_ = progress => {
      const progressBlocked =
          !this.progressTaskToken_ && !!this.delayedCompletionToken_;
      if (!progressBlocked) {
        clearTimeout(this.progressTaskToken_);
        this.progressTaskToken_ = null;
        this.processProgress(progress);
      } else {
        // The progress bar is blocking us for 2000ms
        this.delayedProgress_ = progress;
      }
    };

    // If export started on a different tab, display a busy UI.
    this.passwordManager_.requestExportProgressStatus(status => {
      if (status == ProgressStatus.IN_PROGRESS)
        this.switchToDialog(States.IN_PROGRESS);
    });

    this.passwordManager_.addPasswordsFileExportProgressListener(
        this.onPasswordsFileExportProgressListener_);
  },

  /** Closes the dialog. */
  close: function() {
    clearTimeout(this.progressTaskToken_);
    clearTimeout(this.delayedCompletionToken_);
    this.passwordManager_.removePasswordsFileExportProgressListener(
        this.onPasswordsFileExportProgressListener_);
    this.$.dialog.close();
    this.$.dialog_progress.close();
    this.$.dialog_error.close();
  },

  /**
   * Fires an event that should trigger the password export process.
   * @private
   */
  onExportTap_: function() {
    this.passwordManager_.exportPasswords(() => {
      if (chrome.runtime.lastError &&
          chrome.runtime.lastError.message == 'in-progress') {
        this.switchToDialog(States.IN_PROGRESS);
      }
    });
  },

  /**
   * Acts on export progress events.
   * @param progress {!PasswordManager.PasswordExportProgress}
   * @private
   */
  processProgress(progress) {
    if (progress.status == ProgressStatus.IN_PROGRESS) {
      this.progressTaskToken_ = setTimeout(() => {
        this.progressTask();
      }, progressBarDelay);
      return;
    }
    if (progress.status == ProgressStatus.SUCCEEDED) {
      this.close();
      return;
    }
    if (progress.status == ProgressStatus.FAILED_WRITE_FAILED) {
      this.exportErrorMessage =
          this.i18n('exportPasswordsFailTitle', progress.folderName);
      this.switchToDialog(States.ERROR);
      return;
    }
  },

  /**
   * Opens the specified dialog and hides the others.
   * @param dialog {!States} the dialog to open.
   * @private
   */
  switchToDialog(state) {
    this.$.dialog.open = false;
    this.$.dialog_error.open = false;
    this.$.dialog_progress.open = false;

    if (state == States.START)
      this.$.dialog.showModal();
    if (state == States.ERROR)
      this.$.dialog_error.showModal();
    if (state == States.IN_PROGRESS)
      this.$.dialog_progress.showModal();
  },

  /**
   * Handler for tapping the 'cancel' button. Should just dismiss the dialog.
   * @private
   */
  onCancelButtonTap_: function() {
    this.close();
  },
});
})();