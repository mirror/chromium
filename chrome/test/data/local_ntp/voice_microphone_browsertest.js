// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


/**
 * @fileoverview Tests the microphone module of Voice Search on the local NTP.
 */


/**
 * Voice Search Microphone module's object for test and setup functions.
 */
test.microphone = {};


/**
 * Set up the microphone DOM and test environment.
 */
test.microphone.setUp = function() {
  setUpPage('voice-microphone-template');
  microphone.init();
};


/**
 * Reset the microphone test environment.
 */
test.microphone.tearDown = function() {
  microphone.isLevelAnimating_ = false;
};


/**
 * Makes sure the microphone module sets up with the correct settings.
 */
test.microphone.testInitialization = function() {
  assert(!microphone.isLevelAnimating_);
};


/**
 * Make sure the volume level animation starts.
 */
test.microphone.testStartLevelAnimationFromInactive = function() {
  microphone.startInputAnimation();
  assert(microphone.isLevelAnimating_);
};


/**
 * Make sure the level animation stops.
 */
test.microphone.testStopLevelAnimationFromActive = function() {
  microphone.startInputAnimation();
  microphone.stopInputAnimation();
  assert(!microphone.isLevelAnimating_);
};


/**
 * Make sure the level animation doesn't start again.
 */
test.microphone.testStartLevelAnimationFromActive = function() {
  microphone.startInputAnimation();
  assert(microphone.isLevelAnimating_);
  microphone.startInputAnimation();
  assert(microphone.isLevelAnimating_);
};
