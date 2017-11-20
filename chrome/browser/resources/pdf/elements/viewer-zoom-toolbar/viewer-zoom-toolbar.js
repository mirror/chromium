// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

var FIT_TO_PAGE_BUTTON_STATE = 0;
var FIT_TO_WIDTH_BUTTON_STATE = 1;

Polymer({
  is: 'viewer-zoom-toolbar',

  properties: {
    strings: {type: Object, observer: 'updateTooltips_'},

    visible_: {type: Boolean, value: true}
  },

  isVisible: function() {
    return this.visible_;
  },

  /**
   * @private
   * Change button tooltips to match any changes to localized strings.
   */
  updateTooltips_: function() {
    this.$['fit-button'].tooltips =
        [this.strings.tooltipFitToPage, this.strings.tooltipFitToWidth];
    this.$['zoom-in-button'].tooltips = [this.strings.tooltipZoomIn];
    this.$['zoom-out-button'].tooltips = [this.strings.tooltipZoomOut];
  },

  /**
   * Handle clicks of the fit-button.
   */
  fitToggle: function() {
    if (this.$['fit-button'].activeIndex == FIT_TO_WIDTH_BUTTON_STATE)
      this.fire('fit-to-width');
    else
      this.fire('fit-to-page');
  },

  /**
   * Handle the keyboard shortcut equivalent of fit-button clicks.
   */
  fitToggleFromHotKey: function() {
    this.fitToggle();

    // Toggle the button state since there was no mouse click.
    var button = this.$['fit-button'];
    if (button.activeIndex == FIT_TO_WIDTH_BUTTON_STATE)
      button.activeIndex = FIT_TO_PAGE_BUTTON_STATE;
    else
      button.activeIndex = FIT_TO_WIDTH_BUTTON_STATE;
  },

  /**
   * @private
   * Set the new fit state and also the following fit state upon pressing the
   * fit button.
   */
  forceFitInternal_: function(fitEventToFire, nextButtonState) {
    this.fire(fitEventToFire);

    // Set the button state since there was no mouse click.
    this.$['fit-button'].activeIndex = nextButtonState;
  },

  /**
   * Handle forcing zoom via scripting to a fitting type.
   * @param {Viewport.FittingType} fittingType Page fitting type to force.
   */
  forceFit: function(fittingType) {
    if (fittingType == Viewport.FittingType.FIT_TO_PAGE)
      this.forceFitInternal_('fit-to-page', FIT_TO_WIDTH_BUTTON_STATE);
    else if (fittingType == Viewport.FittingType.FIT_TO_WIDTH)
      this.forceFitInternal_('fit-to-width', FIT_TO_PAGE_BUTTON_STATE);
    else if (fittingType == Viewport.FittingType.FIT_TO_HEIGHT)
      this.forceFitInternal_('fit-to-height', FIT_TO_WIDTH_BUTTON_STATE);
  },

  /**
   * Handle clicks of the zoom-in-button.
   */
  zoomIn: function() {
    this.fire('zoom-in');
  },

  /**
   * Handle clicks of the zoom-out-button.
   */
  zoomOut: function() {
    this.fire('zoom-out');
  },

  show: function() {
    if (!this.visible_) {
      this.visible_ = true;
      this.$['fit-button'].show();
      this.$['zoom-in-button'].show();
      this.$['zoom-out-button'].show();
    }
  },

  hide: function() {
    if (this.visible_) {
      this.visible_ = false;
      this.$['fit-button'].hide();
      this.$['zoom-in-button'].hide();
      this.$['zoom-out-button'].hide();
    }
  },
});

})();
