// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * test funtion
 * @param {string} name someone's name
 */
function jz(name) {
  console.log(name + '7122');
  console.log('yee');
}

class A {
  /**
   * @param {string} name
   */
  constructor(name) {
    /**
     * @private {string}
     */
    this.name = name;
  }
}

/**
 * @type {number}
 */
const name = 5;
var a = new A(/** @type {!string} */ (name));
