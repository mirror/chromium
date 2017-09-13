// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview A class that mediates window timer calls to support mockability
 * in tests.
 */

class TimerProxy {
  /**
   * @param {function()|string} fn
   * @param {number=} delay
   * @return {number}
   */
  setTimeout(fn, delay) {
    return window.setTimeout(fn, delay);
  }

  /** @param {number|undefined?} id */
  clearTimeout(id) {
    window.clearTimeout(id);
  }
}
