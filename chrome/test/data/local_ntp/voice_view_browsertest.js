// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


/**
 * @fileoverview Tests the view module of Voice Search on the local NTP.
 */


// ******************************* SIMPLE TESTS *******************************
// These are run by runSimpleTests above.
// Functions from test_utils.js are automatically imported.


/**
 * The set of textual strings for different states.
 * @enum {string}
 */
const Text = {
  INITIALISING: 'ready',
  LISTENING: 'listening',
  BLANK: ''
};


/**
 * The state of animations.
 * @enum {string}
 */
const Animation = {
  ACTIVE: 'active',
  INACTIVE: 'inactive'
};


/**
 * Variable to indicate whether level animations are underway.
 * @type {string}
 */
let levelState;


/**
 * The interim / low confidence speech recognition result element.
 * @type {string}
 */
let interimText;


/**
 * The final / high confidence speech recognition result element.
 * @type {string}
 */
let finalText;


/**
 * The state that is affected by the view.
 * @type {Object}
 */
let state;

let stubs_;


function setUp() {
  setUpPage('voice-speech-template');
  levelState = Animation.INACTIVE;
  state = {};
  stubs_ = new Replacer();

  stubs_.replace(text, 'cancelListeningTimeout', function() {
    interimText = Text.BLANK;
    finalText = Text.BLANK;
  });
  stubs_.replace(text, 'showInitializingMessage', function() {
    interimText = Text.INITIALISING;
    finalText = Text.BLANK;
  });
  stubs_.replace(text, 'showReadyMessage', function() {
    interimText = Text.LISTENING;
    finalText = Text.BLANK;
  });
  stubs_.replace(text, 'updateTextArea', function(texti, textf) {
    interimText = texti;
    finalText = textf;
  });
  stubs_.replace(text, 'clear', function() {
    interimText = Text.BLANK;
    finalText = Text.BLANK;
  });
  stubs_.replace(microphone, 'startInputAnimation', function() {
    levelState = Animation.ACTIVE;
  });
  stubs_.replace(microphone, 'stopInputAnimation', function() {
    levelState = Animation.INACTIVE;
  });

  view.init(function(shouldSubmit, shouldRetry) {
    state.shouldSubmit = shouldSubmit;
    state.shouldRetry = shouldRetry;
  });
}


function tearDown() {
  view.hide();
  view.isNoMatchShown_ = false;
  view.isVisible_ = false;
  stubs_.reset();
}


/**
 * Makes sure the view sets up with the correct settings.
 */
function testInit() {
  assert(!view.isVisible_);
  assert(!view.isNoMatchShown_);
  assertEquals(Animation.INACTIVE, levelState);
  assertEquals(Text.BLANK, interimText);
  assertEquals(Text.BLANK, finalText);
  assertEquals(view.INACTIVE_CLASS_, view.container_.className);
}


/**
 * Test showing the UI when on the homepage.
 */
function testShowWithReadyElements() {
  view.show();
  assert(view.isVisible_);
  assertEquals(Text.INITIALISING, interimText);
  assertEquals(Text.BLANK, finalText);
  assertEquals(Animation.INACTIVE, levelState);
  assertEquals(view.OVERLAY_CLASS_, view.background_.className);
  assertEquals(view.INACTIVE_CLASS_, view.container_.className);
}


/**
 * Test that trying to show the UI twice doesn't change the
 * view.
 */
function testShowCalledTwice() {
  view.show();
  view.show();
  assert(view.isVisible_);
  assertEquals(Text.INITIALISING, interimText);
  assertEquals(Text.BLANK, finalText);
  assertEquals(Animation.INACTIVE, levelState);
  assertEquals(view.OVERLAY_CLASS_, view.background_.className);
  assertEquals(view.INACTIVE_CLASS_, view.container_.className);
}


/**
 * Test that hiding the UI twice doesn't change the view.
 */
function testHideCalledTwiceAfterShow() {
  view.show();
  view.hide();
  assertEquals(view.OVERLAY_HIDDEN_CLASS_, view.background_.className);
  assertViewInactive();
  view.hide();
  assertEquals(view.OVERLAY_HIDDEN_CLASS_, view.background_.className);
  assertViewInactive();
}


/**
 * Test showing the pulsing animation and 'listening' text.
 */
function testAudioDeviceReady() {
  view.show();
  view.setReadyForSpeech();
  assert(view.isVisible_);
  assertEquals(Text.LISTENING, interimText);
  assertEquals(Text.BLANK, finalText);
  assertEquals(Animation.INACTIVE, levelState);
}


/**
 * Test that the listening text is not shown and the animations are not
 * started if the ui hasn't been started.
 */
function testAudioDeviceListeningBeforeViewStart() {
  view.setReadyForSpeech();
  assertViewInactive();
}


/**
 * Test the volume level animation after starting the view.
 */
function testSpeechStartWithWorkingViews() {
  view.show();
  view.setReceivingSpeech();
  assert(view.isVisible_);
  assertEquals(Text.BLANK, interimText);
  assertEquals(Text.BLANK, finalText);
  assertEquals(Animation.ACTIVE, levelState);
  assertEquals(view.RECEIVING_SPEECH_CLASS_, view.container_.className);
}


/**
 * Test that the output text updates.
 */
function testTextUpdateWithWorkingViews() {
  view.show();
  view.updateSpeechResult('interim', 'final');
  assert(view.isVisible_);
  assertEquals('interim', interimText);
  assertEquals('final', finalText);
  assertEquals(Animation.INACTIVE, levelState);
}


/**
 * Test that starting again after updating the output text doesn't change the
 * view state. This forces hide() to be called to restart the UI.
 */
function testShowCalledAfterUpdate() {
  view.show();
  view.updateSpeechResult('interim', 'final');
  view.show();
  assert(view.isVisible_);
  assertEquals('interim', interimText);
  assertEquals('final', finalText);
  assertEquals(Animation.INACTIVE, levelState);
  assertEquals(view.OVERLAY_CLASS_, view.background_.className);
}


/**
 * Test the typical flow for the view.
 */
function testTypicalFlowWithWorkingViews() {
  assertViewInactive();
  view.show();
  view.setReadyForSpeech();
  view.setReceivingSpeech();
  view.updateSpeechResult('interim1', 'final1');
  assertViewFullActive('interim1', 'final1', view.OVERLAY_CLASS_);
  view.updateSpeechResult('interim2', 'final2');
  assertViewFullActive('interim2', 'final2', view.OVERLAY_CLASS_);
  view.hide();
  assertViewInactive();
}


/**
 * Test hiding the UI after showing it.
 */
function testStopAfterStart() {
  view.show();
  view.hide();
  assertViewInactive();
}


/**
 * Test hiding the UI after audio start.
 */
function testHideAfterAudioStart() {
  view.show();
  view.setReadyForSpeech();
  view.hide();
  assertViewInactive();
}


/**
 * Test hiding the UI after showing speech results.
 */
function testHideAfterSpeechTranscriptReceived() {
  view.show();
  view.setReadyForSpeech();
  view.setReceivingSpeech();
  view.updateSpeechResult('interim', 'final');
  view.hide();
  assertViewInactive();
}


/**
 * Test hiding the UI after speech start.
 */
function testHideAfterSpeechStart() {
  view.show();
  view.setReadyForSpeech();
  view.setReceivingSpeech();
  view.hide();
  assertViewInactive();
}


/**
 * Test that clicking the microphone button when the 'Didn't get that' message
 * has been shown sends the correct parameters to the controller.
 */
function testClickMicToRetryWithNoMatch() {
  view.show();
  view.isNoMatchShown_ = true;

  const fakeClickEvent = {target: {id: microphone.RED_BUTTON_ID}};
  view.onWindowClick_(fakeClickEvent);

  assert(state.shouldRetry);
  assert(!state.shouldSubmit);
}


/**
 * Test that clicking the retry link when the 'Didn't get that' message has been
 * shown sends the correct parameters to the controller.
 */
function testClickLinkToRetryWithNoMatch() {
  view.show();
  view.isNoMatchShown_ = true;

  const fakeClickEvent = {target: {id: text.ERROR_LINK_ID}};
  view.onWindowClick_(fakeClickEvent);

  assert(state.shouldRetry);
  assert(!state.shouldSubmit);
}


/**
 * Test that clicking the microphone button when the 'Didn't get that' message
 * is not shown sends the correct parameters to the controller.
 */
function testClickMicToRetryWithoutNoMatch() {
  view.show();

  const fakeClickEvent = {target: {id: microphone.RED_BUTTON_ID}};
  view.onWindowClick_(fakeClickEvent);

  assert(!state.shouldRetry);
  assert(state.shouldSubmit);
}


/**
 * Test that clicking the retry link when the 'Didn't get that' message is not
 * shown sends the correct shouldRetry parameter to the controller.
 */
function testClickLinkToRetryWithoutNoMatch() {
  view.show();

  const fakeClickEvent = {target: {id: view.ERROR_LINK}};
  view.onWindowClick_(fakeClickEvent);

  assert(!state.shouldRetry);
  assert(!state.shouldSubmit);
}


/**
 * Tests to make sure no components of the view are active.
 */
function assertViewInactive() {
  assert(!view.isVisible_);
  assertEquals(Text.BLANK, interimText);
  assertEquals(Text.BLANK, finalText);
  assertEquals(Animation.INACTIVE, levelState);
  assertEquals(view.INACTIVE_CLASS_, view.container_.className);
}


/**
 * Tests to make sure the all components of the view are working correctly.
 * @param {string} expectedInterimText The expected low confidence text string.
 * @param {string} expectedFinalText The expected high confidence text string.
 * @param {string} expectedPageClass The expected class name of the background_
 *    element.
 */
function assertViewFullActive(
    expectedInterimText, expectedFinalText, expectedPageClass) {
  assert(view.isVisible_);
  assertEquals(expectedPageClass, view.background_.className);
  assertEquals(expectedInterimText, interimText);
  assertEquals(expectedFinalText, finalText);
  assertEquals(Animation.ACTIVE, levelState);
}
