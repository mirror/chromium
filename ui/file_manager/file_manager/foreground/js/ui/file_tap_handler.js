// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Processes touch events and calls back upon tap, longpress and longtap.
 * This class is similar to cr.ui.TouchHandler. The major difference is that,
 * the user of this class can choose to either handle the event as a tap
 * distincted from mouse clicks, or leave it handled by the mouse event
 * handlers by default.
 *
 * @constructor
 */
function FileTapHandler() {
  /**
   * Whether the pointer is currently down and at the same place as the initial
   * position.
   * @type {boolean}
   * @private
   */
  this.tapStarted_ = false;

  /**
   * @type {boolean}
   * @private
   */
  this.isLongTap_ = false;

  /**
   * @type {boolean}
   * @private
   */
  this.hasLongPressProcessed_ = false;

  /**
   * @type {number}
   * @private
   */
  this.longTapDetectorTimerId_ = -1;
}

/**
 * The minimum duration of a tap to be recognized as long press and long tap.
 * This should be consistent with the Views of Android.
 * https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/view/ViewConfiguration.java
 * Also this should also be consistent with Chrome's behavior for issuing
 * drag-and-drop events by touchscreen.
 * @type {number}
 * @const
 */
FileTapHandler.LONG_PRESS_THRESHOLD_MILLISECONDS = 500;

/**
 * @enum {string}
 * @const
 */
FileTapHandler.TapEvent = {
  TAP: 'tap',
  LONG_PRESS: 'longpress',
  LONG_TAP: 'longtap'
};

/**
 * Handles touch events.
 * The propagation of the |event| will be cancelled if the |callback| takes any
 * action, so as to avoid receiving mouse click events for the tapping and
 * processing them duplicatedly.
 * @param {!Event} event a touch event.
 * @param {number} index of the target item in the file list.
 * @param {function(!Event, number, !FileTapHandler.TapEvent)} callback called
 *     when a tap event is detected. Should return ture if it has taken any
 *     action, and false if it ignroes the event.
 */
FileTapHandler.prototype.handleTouchEvents = function(event, index, callback) {
  switch (event.type) {
    case 'touchstart':
      this.tapStarted_ = true;
      this.isLongTap_ = false;
      this.hasLongPressProcessed_ = false;
      this.longTapDetectorTimerId_ = setTimeout(function() {
        this.isLongTap_ = true;
        this.longTapDetectorTimerId_ = -1;
        if (callback(event, index, FileTapHandler.TapEvent.LONG_PRESS)) {
          this.hasLongPressProcessed_ = true;
        }
      }.bind(this), FileTapHandler.LONG_PRESS_THRESHOLD_MILLISECONDS);
      break;
    case 'touchmove':
      // If the pointer is slided, it is a drag. It is no longer a tap.
      this.tapStarted_ = false;
      break;
    case 'touchend':
      if (this.longTapDetectorTimerId_ != -1) {
        clearTimeout(this.longTapDetectorTimerId_);
        this.longTapDetectorTimerId_ = -1;
      }
      if (!this.tapStarted_)
        break;
      if (this.isLongTap_) {
        if (this.hasLongPressProcessed_ ||
            callback(event, index, FileTapHandler.TapEvent.LONG_TAP)) {
          event.preventDefault();
        }
      } else {
        if (callback(event, index, FileTapHandler.TapEvent.TAP)) {
          event.preventDefault();
        }
      }
      break;
  }
};
