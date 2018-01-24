// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview PasswordManager is an abstraction over chrome.passwordsPrivate
 * which facilitates testing.
 */

/**
 * Interface for all callbacks to the password API.
 * @interface
 */
class PasswordManager {
  /**
   * Add an observer to the list of saved passwords.
   * @param {function(!Array<!PasswordManager.PasswordUiEntry>):void} listener
   */
  addSavedPasswordListChangedListener(listener) {}

  /**
   * Remove an observer from the list of saved passwords.
   * @param {function(!Array<!PasswordManager.PasswordUiEntry>):void} listener
   */
  removeSavedPasswordListChangedListener(listener) {}

  /**
   * Request the list of saved passwords.
   * @param {function(!Array<!PasswordManager.PasswordUiEntry>):void} callback
   */
  getSavedPasswordList(callback) {}

  /**
   * Should remove the saved password and notify that the list has changed.
   * @param {number} index The index for the password entry being removed.
   *     No-op if |index| is not in the list.
   */
  removeSavedPassword(index) {}

  /**
   * Add an observer to the list of password exceptions.
   * @param {function(!Array<!PasswordManager.ExceptionEntry>):void} listener
   */
  addExceptionListChangedListener(listener) {}

  /**
   * Remove an observer from the list of password exceptions.
   * @param {function(!Array<!PasswordManager.ExceptionEntry>):void} listener
   */
  removeExceptionListChangedListener(listener) {}

  /**
   * Request the list of password exceptions.
   * @param {function(!Array<!PasswordManager.ExceptionEntry>):void} callback
   */
  getExceptionList(callback) {}

  /**
   * Should remove the password exception and notify that the list has changed.
   * @param {number} index The index for the exception url entry being removed.
   *     No-op if |index| is not in the list.
   */
  removeException(index) {}

  /**
   * Should undo the last saved password or exception removal and notify that
   * the list has changed.
   */
  undoRemoveSavedPasswordOrException() {}

  /**
   * Gets the saved password for a given login pair.
   * @param {number} index The index for password entry that should be
   *     retrieved. No-op if |index| is not in the list.
   * @param {function(!PasswordManager.PlaintextPasswordEvent):void} callback
   */
  getPlaintextPassword(index, callback) {}

  /**
   * Triggers the dialogue for importing passwords.
   */
  importPasswords() {}

  /**
   * Triggers the dialogue for exporting passwords.
   * @param {function():void} callback
   */
  exportPasswords(callback) {}

  /**
   * Queries the status of any ongoing export.
   * @param {function(!PasswordManager.ExportProgressStatus):void} callback
   */
  requestExportProgressStatus(callback) {}

  /**
   * Add an observer to the export progress.
   * @param {function(!PasswordManager.PasswordExportProgress):void} listener
   */
  addPasswordsFileExportProgressListener(listener) {}

  /**
   * Remove an observer from the export progress.
   * @param {function(!PasswordManager.PasswordExportProgress):void} listener
   */
  removePasswordsFileExportProgressListener(listener) {}
}

/** @typedef {chrome.passwordsPrivate.PasswordUiEntry} */
PasswordManager.PasswordUiEntry;

/** @typedef {chrome.passwordsPrivate.LoginPair} */
PasswordManager.LoginPair;

/** @typedef {chrome.passwordsPrivate.ExceptionEntry} */
PasswordManager.ExceptionEntry;

/** @typedef {chrome.passwordsPrivate.PlaintextPasswordEventParameters} */
PasswordManager.PlaintextPasswordEvent;

/** @typedef {{ entry: !PasswordManager.PasswordUiEntry, password: string }} */
PasswordManager.UiEntryWithPassword;

/** @typedef {chrome.passwordsPrivate.PasswordExportProgress} */
PasswordManager.PasswordExportProgress;

/** @typedef {chrome.passwordsPrivate.ExportProgressStatus} */
PasswordManager.ExportProgressStatus;

/**
 * Implementation that accesses the private API.
 * @implements {PasswordManager}
 */
class PasswordManagerImpl {
  /** @override */
  addSavedPasswordListChangedListener(listener) {
    chrome.passwordsPrivate.onSavedPasswordsListChanged.addListener(listener);
  }

  /** @override */
  removeSavedPasswordListChangedListener(listener) {
    chrome.passwordsPrivate.onSavedPasswordsListChanged.removeListener(
        listener);
  }

  /** @override */
  getSavedPasswordList(callback) {
    chrome.passwordsPrivate.getSavedPasswordList(callback);
  }

  /** @override */
  removeSavedPassword(index) {
    chrome.passwordsPrivate.removeSavedPassword(index);
  }

  /** @override */
  addExceptionListChangedListener(listener) {
    chrome.passwordsPrivate.onPasswordExceptionsListChanged.addListener(
        listener);
  }

  /** @override */
  removeExceptionListChangedListener(listener) {
    chrome.passwordsPrivate.onPasswordExceptionsListChanged.removeListener(
        listener);
  }

  /** @override */
  getExceptionList(callback) {
    chrome.passwordsPrivate.getPasswordExceptionList(callback);
  }

  /** @override */
  removeException(index) {
    chrome.passwordsPrivate.removePasswordException(index);
  }

  /** @override */
  undoRemoveSavedPasswordOrException() {
    chrome.passwordsPrivate.undoRemoveSavedPasswordOrException();
  }

  /** @override */
  getPlaintextPassword(index, callback) {
    const listener = function(reply) {
      // Only handle the reply for our loginPair request.
      if (reply.index == index) {
        chrome.passwordsPrivate.onPlaintextPasswordRetrieved.removeListener(
            listener);
        callback(reply);
      }
    };
    chrome.passwordsPrivate.onPlaintextPasswordRetrieved.addListener(listener);
    chrome.passwordsPrivate.requestPlaintextPassword(index);
  }

  /** @override */
  importPasswords() {
    chrome.passwordsPrivate.importPasswords();
  }

  /** @override */
  exportPasswords(callback) {
    chrome.passwordsPrivate.exportPasswords(callback);
  }

  /** @override */
  requestExportProgressStatus(callback) {
    chrome.passwordsPrivate.requestExportProgressStatus(callback);
  }

  /** @override */
  addPasswordsFileExportProgressListener(listener) {
    chrome.passwordsPrivate.onPasswordsFileExportProgress.addListener(listener);
  }

  /** @override */
  removePasswordsFileExportProgressListener(listener) {
    chrome.passwordsPrivate.onPasswordsFileExportProgress.removeListener(
        listener);
  }
}

cr.addSingletonGetter(PasswordManagerImpl);
