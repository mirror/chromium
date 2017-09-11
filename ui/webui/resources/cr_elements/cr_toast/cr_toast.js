// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/**
 * @fileoverview A lightweight toast.
 */
Polymer({
  is: 'cr-toast',

  properties: {
    duration: {
      type: Number,
      value: 0,
    },

    /** @private */
    open_: {
      type: Boolean,
      observer: 'openChanged_',
      value: false,
    },
  },

  /** @private {TimerProxy} */
  timerProxy_: new TimerProxy(),

  /** @private {number|null} */
  hideTimeoutId_: null,

  /** @param {TimerProxy} timerProxy */
  setTimerProxyForTesting: function(timerProxy) {
    this.timerProxy_ = timerProxy;
  },

  show: function() {
    this.open_ = true;

    if (!this.duration)
      return;

    if (this.hideTimeoutId_ != null)
      this.timerProxy_.clearTimeout(this.hideTimeoutId_);

    this.hideTimeoutId_ = this.timerProxy_.setTimeout(() => {
      this.hide();
      this.hideTimeoutId_ = null;
    }, this.duration);
  },

  hide: function() {
    this.open_ = false;
  },

  /** @private */
  openChanged_: function() {
    this.setAttribute('aria-hidden', !this.open_);
  },
});
