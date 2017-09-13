// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

class TestTimerProxy {
  constructor() {
    this.immediatelyResolveTimeouts = true;

    /** @private */
    this.timeoutIds_ = 0;

    /** @private {!Map<number, !Function>} */
    this.activeTimeouts_ = new Map();
  }

  /**
   * @param {Function|string} fn
   * @param {number=} delay
   * @return {number}
   * @override
   */
  setTimeout(fn, delay) {
    if (this.immediatelyResolveTimeouts)
      fn();
    else
      this.activeTimeouts_[this.timeoutIds_] = fn;

    return this.timeoutIds_++;
  }

  /**
   * @param {number|undefined?} id
   * @override
   */
  clearTimeout(id) {
    this.activeTimeouts_.delete(id);
  }

  /**
   * Run the function associated with a timeout id and clear it from the
   * active timeouts.
   * @param {number} id
   */
  runTimeoutFn(id) {
    this.activeTimeouts_[id]();
    this.clearTimeout(id);
  }

  /**
   * Returns true if a given timeout id has not been run or cleared.
   * @param {number} id
   * @return {boolean}
   */
  hasTimeout(id) {
    return this.activeTimeouts_.has(id);
  }
}
