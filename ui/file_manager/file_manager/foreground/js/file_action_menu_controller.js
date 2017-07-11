// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {!cr.ui.MenuButton} fileActionButton
 * @param {!FilesToggleRipple} toggleRipple
 * @param {!cr.ui.Menu} menu
 * @param {!DirectoryModel} directoryModel
 * @param {!CommandHandler} commandHandler
 * @constructor
 * @struct
 */
function FileActionMenuController(
    fileActionButton, toggleRipple, menu, directoryModel, commandHandler) {
  /**
   * @type {!FilesToggleRipple}
   * @const
   * @private
   */
  this.toggleRipple_ = toggleRipple;

  /**
   * @type {!cr.ui.Menu}
   * @const
   */
  this.menu_ = menu;

  /**
   * @type {!DirectoryModel}
   * @const
   * @private
   */
  this.directoryModel_ = directoryModel;

  /**
   * @type {!CommandHandler}
   * @const
   * @private
   */
  this.commandHandler_ = commandHandler;

  /**
   * Subset of items in the context menu which we hide when it's displayed by
   * the button in the action menu.
   *
   * @type {!Array.<string>}
   * @const
   * @private
   */
  this.itemsSelectorsToHide_ = [
    'cr-menu-item[command="#delete"]',
    'cr-menu-item[command="#share"]',
    'cr-menu-item[command="#new-folder"]',
    'cr-menu-item[command="#default-task"]',
    'cr-menu-item[command="#open-with"]',
    'cr-menu-item[command="#paste"]',
    'hr#actions-separator',
    'hr#tasks-separator',
    'hr#file-and-directory-commands-separator',
  ];

  fileActionButton.addEventListener(
      'menushow', this.onShowFileActionMenu_.bind(this));
  fileActionButton.addEventListener(
      'menuhide', this.onHideFileActionMenu_.bind(this));
  directoryModel.addEventListener(
      'directory-changed', this.onDirectoryChanged_.bind(this));
  chrome.fileManagerPrivate.onPreferencesChanged.addListener(
      this.onPreferencesChanged_.bind(this));
  this.onPreferencesChanged_();
}

/**
 * @private
 */
FileActionMenuController.prototype.onShowFileActionMenu_ = function() {
  this.toggleRipple_.activated = true;
  this.itemsSelectorsToHide_.forEach(function(selector) {
    var element = this.menu_.querySelector(selector);
    if (element)
      element.hidden = true;
  }.bind(this));
};

/**
 * @private
 */
FileActionMenuController.prototype.onHideFileActionMenu_ = function() {
  this.toggleRipple_.activated = false;
  this.itemsSelectorsToHide_.forEach(function(selector) {
    var element = this.menu_.querySelector(selector);
    if (element)
      element.hidden = false;
  }.bind(this));
};

/**
 * @param {Event} event
 * @private
 */
FileActionMenuController.prototype.onDirectoryChanged_ = function(event) {
  event = /** @type {DirectoryChangeEvent} */ (event);
  // delete this handler, because trapping selection change will be suffice
};

/**
 * Handles preferences change and updates menu.
 * @private
 */
FileActionMenuController.prototype.onPreferencesChanged_ = function() {
};
