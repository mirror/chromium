// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


/**
 * @fileoverview Tests the microphone module of Voice Search on the local NTP.
 */


// ******************************* SIMPLE TESTS *******************************
// These are run by runSimpleTests above.
// Functions from test_utils.js are automatically imported.


function setUp() {
  setUpPage('voice-microphone-template');
  microphone.init();
}


function tearDown() {
  microphone.isLevelAnimating_ = false;
}


/**
 * Makes sure the microphone module sets up with the correct settings.
 */
function testInitialization() {
  assert(!microphone.isLevelAnimating_);
}


/**
 * Make sure the volume level animation starts.
 */
function testStartLevelAnimationFromInactive() {
  microphone.startInputAnimation();
  assert(microphone.isLevelAnimating_);
}


/**
 * Make sure the level animation stops.
 */
function testStopLevelAnimationFromActive() {
  microphone.startInputAnimation();
  microphone.stopInputAnimation();
  assert(!microphone.isLevelAnimating_);
}


/**
 * Make sure the level animation doesn't start again.
 */
function testStartLevelAnimationFromActive() {
  microphone.startInputAnimation();
  assert(microphone.isLevelAnimating_);
  microphone.startInputAnimation();
  assert(microphone.isLevelAnimating_);
}
