// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Login UI header bar implementation. The header bar is shown on
 * top of login UI (not to be confused with login.HeaderBar which is shown at
 * the bottom and contains login shelf elements).
 * It contains device version labels, and new-note action button if it's
 * enabled (which should only be the case on lock screen).
 */

cr.define('login', function() {

  /**
   * @param element {!HTMLElement}
   * @param swipeCallback {!function(Array<number>)}
   */
  function SwipeDetector(element, swipeCallback) {
    element.addEventListener('touchstart', this.onTouchMove_.bind(this));
    element.addEventListener('touchend', this.onTouchEnd_.bind(this));
    element.addEventListener('touchcancel', this.cancelSwipe_.bind(this));
    element.addEventListener('touchmove', this.onTouchMove_.bind(this));

    /** @private {!function(Array<number>)} */
    this.swipeCallback_ = swipeCallback;
    this.element_ = element;
  }

  function TouchInfo(touch) {
    this.x = touch.clientX;
    this.y = touch.clientY;
    this.time = Date.now();
  }

  SwipeDetector.prototype = {
    /**
     * @private {?{start: !TouchInfo, end: !TouchInfo}}
     */
    current_: null,
    /**
     * @private {?TouchInfo}
     */
    nextSrc_: null,

    /** @private {number} */
    touchId_: null,

    /** @private {number} */
    swipeTimeoutRequest_: undefined,

    /** @private {boolean} */
    enabled_: false,

    /**
     * @param enabled {boolean}
     */
    setEnabled: function(enabled) {
      if (this.enabled_ == enabled)
        return;
      if (!enabled)
        this.cancelSwipe_();
      this.enabled_ = enabled;
    },

    /**
     * @return {boolean}
     */
    started_: function() {
      return !!this.nextStart_;
    },

    /**
     * @param {!TouchInfo} touch
     */
    addTouch_: function(touch) {
      if (this.nextStart_) {
        if (Math.abs(this.nextStart_.x - touch.x) +
                Math.abs(this.nextStart_.y - touch.y) <
            5) {
          this.nextStart_.time = touch.time;
          return;
        }
        this.current_ = {start: this.nextStart_, end: touch};
      }
      this.nextStart_ = touch;
    },

    clearTrace_: function() {
      if (!this.started_()) {
        this.swipeTimeoutRequest_ = undefined;
        return;
      }

      var now = Date.now();

      var isValid = (function(touch) {
                      var timeDelta = now - touch.time;
                      return timeDelta < 300 && timeDelta >= 0;
                    }).bind(this);

      var timeout = -1;
      if (this.current_ && isValid(this.current_.start)) {
        timeout = now - this.current_.start.time;
      } else if (isValid(this.nextStart_)) {
        this.current_ = null;
        timeout = now - this.nextStart_.time;
      } else {
        this.cancelSwipe_();
        return;
      }

      this.swipeTimeoutRequest_ =
          setTimeout(this.clearTrace_.bind(this), timeout);
    },

    cancelSwipe_: function() {
      if (this.swipeTimeoutRequest_ !== undefined)
        clearTimeout(this.swipeTimeoutRequest_);
      this.swipeTimeoutRequest_ = undefined;
      this.touchId_ = null;
      this.current_ = null;
      this.nextStart_ = null;
    },

    /**
     * @param {TouchEvent} e
     */
    onTouchMove_: function(e) {
      if (!this.enabled_)
        return;

      if (e.touches.length > 1) {
        this.cancelSwipe_();
        return;
      }

      var touch = e.touches[0];
      if (!this.started_() || touch.identifier != this.touchId_) {
        this.cancelSwipe_();

        this.swipeTimeoutRequest_ =
            setTimeout(this.clearTrace_.bind(this), 300);

        this.touchId_ = touch.identifier;
      }

      this.addTouch_(new TouchInfo(touch));
    },

    /** @param {TouchEvent} e */
    onTouchEnd_: function(e) {
      if (!this.enabled_)
        return;

      if (e.changedTouches.length > 1 ||
          e.changedTouches[0].identifier != this.touchId_) {
        this.cancelSwipe_();
        return;
      }

      this.addTouch_(new TouchInfo(e.changedTouches[0]));

      var current = this.current_;
      this.cancelSwipe_();

      if (!current)
        return;

      var velocity = {
        x: current.end.x - current.start.x,
        y: current.end.y - current.start.y
      };
      if (!velocity.x && !velocity.y)
        return;

      var time = current.end.time - current.start.time;
      if (!time)
        return;

      this.swipeCallback_({x: velocity.x / time, y: velocity.y / time});
    }
  };

  /**
   * Creates a header bar element shown at the top of the login screen.
   *
   * @constructor
   * @extends {HTMLDivElement}
   */
  var TopHeaderBar = cr.ui.define('div');

  /**
   * Calculates diagonal length of a rectangle with the provided sides.
   * @param {!number} x The rectangle width.
   * @param {!number} y The rectangle height.
   * @return {!number} The rectangle diagonal.
   */
  function diag(x, y) {
    return Math.sqrt(x * x + y * y);
  }

  TopHeaderBar.prototype = {
    __proto__: HTMLDivElement.prototype,

    /**
     * The current state of lock screen apps.
     * @private {!LockScreenAppsState}
     */
    lockScreenAppsState_: LOCK_SCREEN_APPS_STATE.NONE,

    /** @private {SwipeDetector} */
    swipeDetector_: null,

    set lockScreenAppsState(state) {
      if (this.lockScreenAppsState_ == state)
        return;

      this.lockScreenAppsState_ = state;
      this.updateUi_();
    },

    /** override */
    decorate: function() {
      $('new-note-action')
          .addEventListener('click', this.activateNoteAction_.bind(this));
      $('new-note-action')
          .addEventListener(
              'keydown', this.handleNewNoteActionKeyDown_.bind(this));
      this.swipeDetector_ =
          new SwipeDetector($('new-note-action'), this.handleSwipe_.bind(this));
    },

    /**
     * @private
     */
    updateUi_: function() {
      // Shorten transition duration when moving to available state, in
      // particular when moving from foreground state - when moving from
      // foreground state container gets cropped to top right corner
      // immediately, while action element is updated from full screen with
      // a transition. If the transition is too long, the action would be
      // displayed as full square for a fraction of a second, which can look
      // janky.
      if (this.lockScreenAppsState_ == LOCK_SCREEN_APPS_STATE.AVAILABLE) {
        $('new-note-action')
            .classList.toggle('new-note-action-short-transition', true);
        this.runOnNoteActionTransitionEnd_(function() {
          $('new-note-action')
              .classList.toggle('new-note-action-short-transition', false);
        });
      }

      this.swipeDetector_.setEnabled(
          this.lockScreenAppsState_ == LOCK_SCREEN_APPS_STATE.AVAILABLE);

      $('top-header-bar')
          .classList.toggle(
              'version-labels-unset',
              this.lockScreenAppsState_ == LOCK_SCREEN_APPS_STATE.FOREGROUND ||
                  this.lockScreenAppsState_ ==
                      LOCK_SCREEN_APPS_STATE.BACKGROUND);

      $('new-note-action-container').hidden =
          this.lockScreenAppsState_ != LOCK_SCREEN_APPS_STATE.AVAILABLE &&
          this.lockScreenAppsState_ != LOCK_SCREEN_APPS_STATE.FOREGROUND;

      $('new-note-action')
          .classList.toggle(
              'disabled',
              this.lockScreenAppsState_ != LOCK_SCREEN_APPS_STATE.AVAILABLE);

      $('new-note-action-icon').hidden =
          this.lockScreenAppsState_ != LOCK_SCREEN_APPS_STATE.AVAILABLE;

      // This might get set when the action is activated - reset it when the
      // lock screen action is updated.
      $('new-note-action-container')
          .classList.toggle('new-note-action-above-login-header', false);

      if (this.lockScreenAppsState_ != LOCK_SCREEN_APPS_STATE.FOREGROUND) {
        // Reset properties that might have been set as a result of activating
        // new note action.
        $('new-note-action-container')
            .classList.toggle('new-note-action-container-activated', false);
        $('new-note-action').style.removeProperty('border-bottom-left-radius');
        $('new-note-action').style.removeProperty('border-bottom-right-radius');
        $('new-note-action').style.removeProperty('height');
        $('new-note-action').style.removeProperty('width');
      }
    },

    /**
     * Handler for key down event.
     * @param {!KeyboardEvent} evt The key down event.
     * @private
     */
    handleNewNoteActionKeyDown_: function(evt) {
      if (evt.code != 'Enter')
        return;
      this.activateNoteAction_();
    },

    /**
     * @param {!Array<number>} velocity
     */
    handleSwipe_: function(velocity) {
      // Ignore swipes in direction other than down left (in ltr world).
      if (velocity.x >= 0 || velocity.y <= 0)
        return;
      // Not fast enough - ignore.
      var length = diag(velocity.x, velocity.y);
      if (length < 1)
        return;

      this.activateNoteAction_();
    },

    /**
     * @private
     */
    activateNoteAction_: function() {
      $('new-note-action').classList.toggle('disabled', true);
      $('new-note-action-icon').hidden = true;
      $('top-header-bar').classList.toggle('version-labels-unset', true);

      this.runOnNoteActionTransitionEnd_(
          (function() {
            if (this.lockScreenAppsState_ != LOCK_SCREEN_APPS_STATE.AVAILABLE)
              return;
            chrome.send(
                'setLockScreenAppsState',
                [LOCK_SCREEN_APPS_STATE.LAUNCH_REQUESTED]);
          }).bind(this));

      var container = $('new-note-action-container');
      container.classList.toggle('new-note-action-container-activated', true);
      container.classList.toggle('new-note-action-above-login-header', true);

      var newNoteAction = $('new-note-action');
      // Update new-note-action size to cover full screen - the element is a
      // circle quadrant, intent is for the whole quadrant to cover the screen
      // area, which means that the element size (radius) has to be set to the
      // container diagonal. Note that, even though the final new-note-action
      // element UI state is full-screen, the element is kept as circle quadrant
      // for purpose of transition animation (to get the effect of the element
      // radius increasing until it covers the whole screen).
      var targetSize =
          '' + diag(container.clientWidth, container.clientHeight) + 'px';
      newNoteAction.style.setProperty('width', targetSize);
      newNoteAction.style.setProperty('height', targetSize);
      newNoteAction.style.setProperty(
          isRTL() ? 'border-bottom-right-radius' : 'border-bottom-left-radius',
          targetSize);
    },

    /**
     * Waits for new note action element transition to end (the element expands
     * from top right corner to whole screen when the action is activated) and
     * then runs the provided closure.
     *
     * @param {!function()} callback Closure to run on transition end.
     */
    runOnNoteActionTransitionEnd_: function(callback) {
      $('new-note-action').addEventListener('transitionend', function listen() {
        $('new-note-action').removeEventListener('transitionend', listen);
        callback();
      });
      ensureTransitionEndEvent($('new-note-action'));
    }
  };

  return {TopHeaderBar: TopHeaderBar};
});
