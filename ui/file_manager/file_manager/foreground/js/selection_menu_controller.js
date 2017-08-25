// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @param {!cr.ui.MenuButton} selectionMenuButton
 * @param {!cr.ui.Menu} menu
 * @constructor
 * @struct
 */
function SelectionMenuController(selectionMenuButton, menu) {
  /**
   * @type {!FilesToggleRipple}
   * @const
   * @private
   */
  this.toggleRipple_ =
      /** @type {!FilesToggleRipple} */ (
          queryRequiredElement('files-toggle-ripple', selectionMenuButton));

  /**
   * Whether the menu has been activated, or de-activated otherwise.
   * @type {boolean}
   * @private
   */
  this.activated_ = false;

  /**
   * @type {!cr.ui.Menu}
   * @const
   */
  this.menu_ = menu;

  selectionMenuButton.addEventListener('menushow', this.onShowMenu_.bind(this));
  selectionMenuButton.addEventListener('menuhide', this.onHideMenu_.bind(this));
  this.menu_.addEventListener(
      'transitionend', this.onTransitionEnd_.bind(this));
}

/**
 * Class name to indicate if the menu was opened by the toolbar button or not.
 * @type {string}
 * @const
 */
SelectionMenuController.TOOLBAR_MENU = 'toolbar-menu';

/**
 * @private
 */
SelectionMenuController.prototype.onShowMenu_ = function() {
  this.activated_ = true;
  this.menu_.classList.toggle(SelectionMenuController.TOOLBAR_MENU, true);
  this.toggleRipple_.activated = true;
};

/**
 * @private
 */
SelectionMenuController.prototype.onHideMenu_ = function() {
  this.activated_ = false;
  this.toggleRipple_.activated = false;
};

/**
 * @private
 */
SelectionMenuController.prototype.onTransitionEnd_ = function() {
  if (!this.activated_)
    // The fade-out animation has finished.
    // Update menu items in the next cycle to avoid flashing.
    setTimeout(function() {
      this.menu_.classList.toggle(SelectionMenuController.TOOLBAR_MENU, false);
    }.bind(this));
};
