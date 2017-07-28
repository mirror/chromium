// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

{
  var LONG_TOUCH_TIME_MS = 1000;

  function LongTouchDetector(element, callback) {
    this.callback_ = callback;

    element.addEventListener('touchstart', this.onTouchStart_.bind(this));
    element.addEventListener('touchend', this.onTouchEnd_.bind(this));
    element.addEventListener('touchcancel', this.onTouchEnd_.bind(this));

    element.addEventListener('mousedown', this.onTouchStart_.bind(this));
    element.addEventListener('mouseup', this.onTouchEnd_.bind(this));
    element.addEventListener('mouseleave', this.onTouchEnd_.bind(this));
  }

  LongTouchDetector.prototype = {
    // This is ID of detection attempt so that delayed events will not interfere
    // with current detection.
    //
    // @private {number}
    attempt_: 0,

    // This is "current detection ID".
    // If timeout occurs and it doesn't match expected, timeout is itnored.
    // It is null when detection is not active.
    //
    // @private {number|null}
    pending_attempt_: null,

    // window.setTimeout() callback.
    //
    // @param attempt {number} - Expected detection ID.
    // @private
    onTimeout_: function(attempt) {
      if (attempt !== this.pending_attempt_)
        return;
      this.pending_attempt_ = null;
      this.callback_();
    },

    // @private
    onTouchStart_: function() {
      ++this.attempt_;
      this.pending_attempt_ = this.attempt_;
      window.setTimeout(
          this.onTimeout_.bind(this, this.attempt_), LONG_TOUCH_TIME_MS);
    },

    // @private
    onTouchEnd_: function() {
      this.pending_attempt_ = null;
    },
  };

  Polymer({
    is: 'oobe-welcome-dialog',
    properties: {
      /**
       * Currently selected system language (display name).
       */
      currentLanguage: {
        type: String,
        value: '',
      },

      /**
       * Controls visibility of "Timezone" button.
       */
      timezoneButtonVisible: {
        type: Boolean,
        value: false,
      },

      /**
       * Controls displaying of "Enable debugging features" link.
       */
      debuggingLinkVisible: Boolean,
    },

    /*
     * @private {LongTouchDetector}
     */
    titleLongTouchDetector_: null,

    /**
     * This is stored ID of currently focused element to restore id on returns
     * to this dialog from Language / Timezone Selection dialogs.
     */
    focusedElement_: 'languageSelectionButton',

    onLanguageClicked_: function() {
      this.focusedElement_ = 'languageSelectionButton';
      this.fire('language-button-clicked');
    },

    onAccessibilityClicked_: function() {
      this.focusedElement_ = 'accessibilitySettingsButton';
      this.fire('accessibility-button-clicked');
    },

    onTimezoneClicked_: function() {
      this.focusedElement_ = 'timezoneSettingsButton';
      this.fire('timezone-button-clicked');
    },

    onNextClicked_: function() {
      this.focusedElement_ = 'welcomeNextButton';
      this.fire('next-button-clicked');
    },

    onDebuggingLinkClicked_: function() {
      chrome.send(
          'login.NetworkScreen.userActed', ['connect-debugging-features']);
    },

    /*
     * This is called from titleLongTouchDetector_ when long touch is detected.
     *
     * @private
     */
    onTitleLongTouch_: function() {
      this.fire('launch-advanced-options');
    },

    attached: function() {
      this.titleLongTouchDetector_ = new LongTouchDetector(
          this.$.title, this.onTitleLongTouch_.bind(this));
      this.focus();
    },

    focus: function() {
      var focusedElement = this.$[this.focusedElement_];
      if (focusedElement)
        focusedElement.focus();
    },

    /**
     * This is called from oobe_welcome when this dialog is shown.
     */
    show: function() {
      this.focus();
    },

    /**
     * This function formats message for labels.
     * @param String label i18n string ID.
     * @param String parameter i18n string parameter.
     * @private
     */
    formatMessage_: function(label, parameter) {
      return loadTimeData.getStringF(label, parameter);
    },
  });
}
