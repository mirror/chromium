// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {!cr.ui.MenuButton} selectionMenuButton
 * @param {!FilesToggleRipple} toggleRipple
 * @param {!cr.ui.Menu} menu
 * @constructor
 * @struct
 */
function SelectionMenuController(selectionMenuButton, toggleRipple, menu) {
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
   * Subset of items in the context menu which we hide when it's displayed by
   * the button in the action bar.
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

  selectionMenuButton.addEventListener('menushow', this.onShowMenu_.bind(this));
  selectionMenuButton.addEventListener('menuhide', this.onHideMenu_.bind(this));
}

/**
 * @private
 */
SelectionMenuController.prototype.onShowMenu_ = function() {
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
SelectionMenuController.prototype.onHideMenu_ = function() {
  this.toggleRipple_.activated = false;
  this.itemsSelectorsToHide_.forEach(function(selector) {
    var element = this.menu_.querySelector(selector);
    if (element)
      element.hidden = false;
  }.bind(this));
};
