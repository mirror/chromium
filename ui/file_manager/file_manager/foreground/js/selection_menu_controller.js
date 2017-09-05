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
   * Whether the menu is being fade out by the CSS transition.
   * @type {boolean}
   * @private
   */
  this.fadingOut_ = false;

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
  this.menu_.classList.toggle(SelectionMenuController.TOOLBAR_MENU, true);
  this.toggleRipple_.activated = true;
  this.fadingOut_ = false;
};

/**
 * @private
 */
SelectionMenuController.prototype.onHideMenu_ = function() {
  this.toggleRipple_.activated = false;
  this.fadingOut_ = true;
};

/**
 * @private
 * @param {!Event} event The transition event.
 */
SelectionMenuController.prototype.onTransitionEnd_ = function(event) {
  if (!this.fadingOut_ || event.target != this.menu_)
    return;
  // The fade-out animation of the menu box is being finished.
  this.menu_.classList.toggle(SelectionMenuController.TOOLBAR_MENU, false);
  // This event is not invoked right after fading out the menu, but at the
  // beginning of the the next fade-in animation by right-click.
  // TODO(yamaguchi): Trap the transition end of cr.ui.Menu in a different way.
};
