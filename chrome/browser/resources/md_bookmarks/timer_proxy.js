// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A class that mediates window timer calls to support mockability
 * in tests.
 */

cr.define('bookmarks', function() {
  /** @constructor */
  function TimerProxy() {}

  TimerProxy.prototype = {
    /**
     * @param {function()|string} fn
     * @param {number=} delay
     * @return {number}
     */
    setTimeout: function(fn, delay) {
      return window.setTimeout(fn, delay);
    },

    /** @param {number|undefined?} id */
    clearTimeout: function(id) {
      window.clearTimeout(id);
    },
  };

  /**
   * A debouncer which fires the given callback after a delay. The delay can be
   * refreshed by calling resetTimer. Resetting the timeout with no delay moves
   * the callback to the end of the task queue.
   * @param {?function()=} callback
   * @constructor
   */
  function Debouncer(callback) {
    /** @private {?function()} */
    this.callback_ = callback || null;
    /** @private {!bookmarks.TimerProxy} */
    this.timerProxy_ = new TimerProxy();
    /** @private {?number} */
    this.timer_ = null;
    /** @private {!function()} */
    this.boundTimerCallback_ = this.timerCallback_.bind(this);
    /** @private {boolean} */
    this.isDone_ = false;
    /** @private {!PromiseResolver} */
    this.promiseResolver_ = new PromiseResolver();
  }

  Debouncer.prototype = {
    /**
     * Starts the timer for the callback, cancelling the old timer if there is
     * one.
     * @param {number=} delay
     */
    resetTimeout: function(delay) {
      assert(!this.isDone_);
      if (!this.callback_)
        return;

      this.cancelTimeout_();
      this.timer_ =
          this.timerProxy_.setTimeout(this.boundTimerCallback_, delay);
    },

    /**
     * @return {boolean} True if the Debouncer has finished processing.
     */
    done: function() {
      return this.isDone_;
    },

    /**
     * @return {Promise} Promise which resolves immediately after the callback.
     */
    get promise() {
      return this.promiseResolver_.promise;
    },

    /**
     * Sets a new completion callback, resetting the debouncer.
     * @param {!function()} callback
     */
    setCallback: function(callback) {
      this.reset();

      this.callback_ = callback;
    },

    /**
     * Resets the debouncer as if it had been newly instantiated.
     */
    reset: function() {
      this.isDone_ = false;
      this.promiseResolver_ = new PromiseResolver();
      this.cancelTimeout_();
    },

    /**
     * Cancel the timer callback, which can be restarted by calling
     * resetTimeout().
     * @private
     */
    cancelTimeout_: function() {
      if (this.timer_)
        this.timerProxy_.clearTimeout(this.timer_);
    },

    /** @private */
    timerCallback_: function() {
      assert(this.callback_);

      this.isDone_ = true;
      this.callback_.call();
      this.promiseResolver_.resolve();
    },
  };

  return {
    Debouncer: Debouncer,
    TimerProxy: TimerProxy,
  };
});
