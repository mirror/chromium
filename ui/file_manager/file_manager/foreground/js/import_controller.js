// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Namespace
var importer = importer || {};

/** @enum {string} */
importer.ResponseId = {
  HIDDEN: 'hidden',
  SCANNING: 'scanning',
  NO_MEDIA: 'no_media',
  EXECUTABLE: 'executable'
};

/**
 * @typedef {{
 *   id: !importer.ResponseId,
 *   label: string,
 *   visible: boolean,
 *   executable: boolean
 * }}
 */
importer.CommandUpdate;

/**
 * Class that orchestrates background activity and UI changes on
 * behalf of Cloud Import.
 *
 * @constructor
 * @struct
 *
 * @param {!importer.ControllerEnvironment} environment The class providing
 *     access to runtime environmental information, like the current directory,
 *     volume lookup and so-on.
 * @param {!importer.MediaScanner} scanner
 * @param {!importer.ImportRunner} importRunner
 * @param {function()} commandUpdateHandler
 */
importer.ImportController =
    function(environment, scanner, importRunner, commandUpdateHandler) {

  /** @private {!importer.ControllerEnvironment} */
  this.environment_ = environment;

  /** @private {!importer.ImportRunner} */
  this.importRunner_ = importRunner;

  /** @private {!importer.MediaScanner} */
  this.scanner_ = scanner;

  /** @private {function()} */
  this.updateCommands_ = commandUpdateHandler;

  /**
   * A cache of scans by volumeId, directory URL.
   * Currently only scans of directories are cached.
   * @private {!Object.<string, !Object.<string, !importer.ScanResult>>}
   */
  this.cachedScans_ = {};

  this.scanner_.addObserver(this.onScanEvent_.bind(this));

  this.environment_.addVolumeUnmountListener(
      this.onVolumeUnmounted_.bind(this));
};

/**
 * @param {!importer.ScanEvent} event Command event.
 * @param {importer.ScanResult} result
 *
 * @private
 */
importer.ImportController.prototype.onScanEvent_ = function(event, result) {
  // TODO(smckay): only do this if this is a directory scan.
  if (event === importer.ScanEvent.FINALIZED) {
    this.updateCommands_();
  }
};

/**
 * Executes import against the current directory. Should only
 * be called when the current directory has been validated
 * by calling "update" on this class.
 */
importer.ImportController.prototype.execute = function() {
  metrics.recordEnum('CloudImport.UserAction', 'IMPORT_INITIATED');
  var result = this.getScanForImport_();
  var importTask = this.importRunner_.importFromScanResult(result);

  importTask.getDestination().then(
      /**
       * @param {!DirectoryEntry} destination
       */
      function(destination) {
        this.environment_.setCurrentDirectory(destination);
      }.bind(this));
};

/**
 * Called by the 'cloud-import' command when it wants an update
 * on the command state.
 *
 * @return {!importer.CommandUpdate} response
 */
importer.ImportController.prototype.getCommandUpdate = function() {

  // If there is no Google Drive mount, Drive may be disabled
  // or the machine may be running in guest mode.
  if (this.environment_.isGoogleDriveMounted()) {
    var entries = this.environment_.getSelection();

    // Enabled if user has a selection and it consists entirely of files
    // that:
    // 1) are of a recognized media type
    // 2) reside on a removable media device
    // 3) in the DCIM directory
    if (entries.length) {
      if (entries.every(
          importer.isEligibleEntry.bind(null, this.environment_))) {
        return importer.ImportController.createUpdate_(
            importer.ResponseId.EXECUTABLE, entries.length);
      }
    } else if (this.isCurrentDirectoryScannable_()) {
      var scan = this.getCurrentDirectoryScan_();
      if (scan.isFinal()) {
        if (scan.getFileEntries().length === 0) {
          return importer.ImportController.createUpdate_(
              importer.ResponseId.NO_MEDIA);
        } else {
          return importer.ImportController.createUpdate_(
              importer.ResponseId.EXECUTABLE,
              scan.getFileEntries().length);
        }
      } else {
        return importer.ImportController.createUpdate_(
            importer.ResponseId.SCANNING);
      }
    }
  }

  return importer.ImportController.createUpdate_(
      importer.ResponseId.HIDDEN);
};

/**
 * @param {importer.ResponseId} responseId
 * @param {number=} opt_fileCount
 *
 * @return {!importer.CommandUpdate}
 * @private
 */
importer.ImportController.createUpdate_ =
    function(responseId, opt_fileCount) {
  switch(responseId) {
    case importer.ResponseId.HIDDEN:
      return {
        id: responseId,
        visible: false,
        executable: false,
        label: '** SHOULD NOT BE VISIBLE **'
      };
    case importer.ResponseId.SCANNING:
      return {
        id: responseId,
        visible: true,
        executable: false,
        label: str('CLOUD_IMPORT_SCANNING_BUTTON_LABEL')
      };
    case importer.ResponseId.NO_MEDIA:
      return {
        id: responseId,
        visible: true,
        executable: false,
        label: str('CLOUD_IMPORT_EMPTY_SCAN_BUTTON_LABEL')
      };
    case importer.ResponseId.EXECUTABLE:
      return {
        id: responseId,
        label: strf('CLOUD_IMPORT_BUTTON_LABEL', opt_fileCount),
        visible: true,
        executable: true
      };
    default:
      assertNotReached('Unrecognized response id: ' + responseId);
  }
};

/**
 * @return {boolean} true if the current directory is scan eligible.
 * @private
 */
importer.ImportController.prototype.isCurrentDirectoryScannable_ =
    function() {
  var directory = this.environment_.getCurrentDirectory();
  return !!directory &&
      importer.isMediaDirectory(directory, this.environment_);
};

/**
 * Get or create scan for the current directory or file selection.
 *
 * @return {!importer.ScanResult} A scan result object that may be
 *     actively scanning.
 * @private
 */
importer.ImportController.prototype.getScanForImport_ = function() {
  var entries = this.environment_.getSelection();

  if (entries.length) {
    if (entries.every(
        importer.isEligibleEntry.bind(null, this.environment_))) {
      return this.scanner_.scan(entries);
    }
  } else {
    return this.getCurrentDirectoryScan_();
  }
};

/**
 * Get or create scan for the current directory.
 *
 * @return {!importer.ScanResult} A scan result object that may be
 *     actively scanning.
 * @private
 */
importer.ImportController.prototype.getCurrentDirectoryScan_ = function() {
  console.assert(this.isCurrentDirectoryScannable_());
  var directory = this.environment_.getCurrentDirectory();
  var volumeId = this.environment_.getVolumeInfo(directory).volumeId;

  // Lazily initialize the cache for volumeId.
  if (!this.cachedScans_.hasOwnProperty(volumeId)) {
    this.cachedScans_[volumeId] = {};
  }

  var url = directory.toURL();
  var scan = this.cachedScans_[volumeId][url];
  if (!scan) {
    scan = this.scanner_.scan([directory]);
    this.cachedScans_[volumeId][url] = scan;
  }
  return scan;
};

/**
 * @param {string} volumeId
 * @private
 */
importer.ImportController.prototype.onVolumeUnmounted_ = function(volumeId) {
  // Forget all scans related to the unmounted volume volume.
  if (this.cachedScans_.hasOwnProperty(volumeId)) {
    delete this.cachedScans_[volumeId];
  }
};

/**
 * Interface abstracting away the concrete file manager available
 * to commands. By hiding file manager we make it easy to test
 * ImportController.
 *
 * @interface
 * @extends {VolumeManagerCommon.VolumeInfoProvider}
 */
importer.ControllerEnvironment = function() {};

/**
 * Returns the current file selection, if any. May be empty.
 * @return {!Array.<!Entry>}
 */
importer.ControllerEnvironment.prototype.getSelection;

/**
 * Returns the directory entry for the current directory.
 * @return {!DirectoryEntry}
 */
importer.ControllerEnvironment.prototype.getCurrentDirectory;

/**
 * @param {!DirectoryEntry} entry
 */
importer.ControllerEnvironment.prototype.setCurrentDirectory;

/**
 * Returns true if the Drive mount is present.
 * @return {boolean}
 */
importer.ControllerEnvironment.prototype.isGoogleDriveMounted;

/**
 * Installs an 'unmount' listener. Listener is called with
 * the corresponding volume id when a volume is unmounted.
 * @param {function(string)} listener
 */
importer.ControllerEnvironment.prototype.addVolumeUnmountListener;

/**
 * Class providing access to various pieces of information in the
 * FileManager environment, like the current directory, volumeinfo lookup
 * By hiding file manager we make it easy to test importer.ImportController.
 *
 * @constructor
 * @implements {importer.ControllerEnvironment}
 *
 * @param {!FileManager} fileManager
 */
importer.RuntimeControllerEnvironment = function(fileManager) {
  /** @private {!FileManager} */
  this.fileManager_ = fileManager;
};

/** @override */
importer.RuntimeControllerEnvironment.prototype.getSelection =
    function() {
  return this.fileManager_.getSelection().entries;
};

/** @override */
importer.RuntimeControllerEnvironment.prototype.getCurrentDirectory =
    function() {
  return /** @type {!DirectoryEntry} */ (
      this.fileManager_.getCurrentDirectoryEntry());
};

/** @override */
importer.RuntimeControllerEnvironment.prototype.setCurrentDirectory =
    function(entry) {
  this.fileManager_.directoryModel.activateDirectoryEntry(entry);
};

/** @override */
importer.RuntimeControllerEnvironment.prototype.getVolumeInfo =
    function(entry) {
  return this.fileManager_.volumeManager.getVolumeInfo(entry);
};

/** @override */
importer.RuntimeControllerEnvironment.prototype.isGoogleDriveMounted =
    function() {
  var drive = this.fileManager_.volumeManager.getCurrentProfileVolumeInfo(
      VolumeManagerCommon.VolumeType.DRIVE);
  return !!drive;
};

/** @override */
importer.RuntimeControllerEnvironment.prototype.addVolumeUnmountListener =
    function(listener) {
  chrome.fileManagerPrivate.onMountCompleted.addListener(
      /**
       * @param {!MountCompletedEvent} event
       * @this {importer.RuntimeControllerEnvironment}
       */
      function(event) {
        if (event.eventType === 'unmount') {
          listener(event.volumeMetadata.volumeId);
        }
      });
};
