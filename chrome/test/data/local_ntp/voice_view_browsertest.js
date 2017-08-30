// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


/**
 * @fileoverview Tests the view module of Voice Search on the local NTP.
 */


/**
 * Voice Search View module's object for test and setup functions.
 */
test.view = {};


/**
 * The set of textual strings for different states.
 * @enum {string}
 */
test.view.Text = {
  INITIALIZING: 'ready',
  LISTENING: 'listening',
  BLANK: ''
};


/**
 * The state of animations.
 * @enum {string}
 */
test.view.Animation = {
  ACTIVE: 'active',
  INACTIVE: 'inactive'
};


/**
 * Variable to indicate whether level animations are underway.
 * @type {string}
 */
test.view.levelState = '';


/**
 * The interim / low confidence speech recognition result element.
 * @type {string}
 */
test.view.interimText = '';


/**
 * The final / high confidence speech recognition result element.
 * @type {string}
 */
test.view.finalText = '';


/**
 * The state that is affected by the view.
 * @type {Object}
 */
test.view.state = {};


/**
 * Utility to mock out object properties.
 * @type {Replacer}
 */
test.view.stubs = new Replacer();


/**
 * Set up the text DOM and test environment.
 */
test.view.setUp = function() {
  if (!!view.background_) {
    view.hide();
  }
  view.isNoMatchShown_ = false;
  view.isVisible_ = false;

  test.view.levelState = test.view.Animation.INACTIVE;
  test.view.state = {};
  test.view.stubs.reset();

  setUpPage('voice-view-template');

  test.view.stubs.replace(text, 'cancelListeningTimeout', function() {
    test.view.interimText = test.view.Text.BLANK;
    test.view.finalText = test.view.Text.BLANK;
  });
  test.view.stubs.replace(text, 'showInitializingMessage', function() {
    test.view.interimText = test.view.Text.INITIALIZING;
    test.view.finalText = test.view.Text.BLANK;
  });
  test.view.stubs.replace(text, 'showReadyMessage', function() {
    test.view.interimText = test.view.Text.LISTENING;
    test.view.finalText = test.view.Text.BLANK;
  });
  test.view.stubs.replace(text, 'updateTextArea', function(texti, textf) {
    test.view.interimText = texti;
    test.view.finalText = textf;
  });
  test.view.stubs.replace(text, 'clear', function() {
    test.view.interimText = test.view.Text.BLANK;
    test.view.finalText = test.view.Text.BLANK;
  });
  test.view.stubs.replace(microphone, 'startInputAnimation', function() {
    test.view.levelState = test.view.Animation.ACTIVE;
  });
  test.view.stubs.replace(microphone, 'stopInputAnimation', function() {
    test.view.levelState = test.view.Animation.INACTIVE;
  });

  view.init(function(shouldSubmit, shouldRetry) {
    test.view.state.shouldSubmit = shouldSubmit;
    test.view.state.shouldRetry = shouldRetry;
  });
};


/**
 * Makes sure the view sets up with the correct settings.
 */
test.view.testInit = function() {
  test.view.assertViewInactive();
};


/**
 * Test showing the UI when on the homepage.
 */
test.view.testShowWithReadyElements = function() {
  view.show();
  assert(view.isVisible_);
  assertEquals(test.view.Text.INITIALIZING, test.view.interimText);
  assertEquals(test.view.Text.BLANK, test.view.finalText);
  assertEquals(test.view.Animation.INACTIVE, test.view.levelState);
  assertEquals(view.OVERLAY_CLASS_, view.background_.className);
  assertEquals(view.INACTIVE_CLASS_, view.container_.className);
};


/**
 * Test that trying to show the UI twice doesn't change the
 * view.
 */
test.view.testShowCalledTwice = function() {
  view.show();
  view.show();
  assert(view.isVisible_);
  assertEquals(test.view.Text.INITIALIZING, test.view.interimText);
  assertEquals(test.view.Text.BLANK, test.view.finalText);
  assertEquals(test.view.Animation.INACTIVE, test.view.levelState);
  assertEquals(view.OVERLAY_CLASS_, view.background_.className);
  assertEquals(view.INACTIVE_CLASS_, view.container_.className);
};


/**
 * Test that hiding the UI twice doesn't change the view.
 */
test.view.testHideCalledTwiceAfterShow = function() {
  view.show();
  view.hide();
  test.view.assertViewInactive();
  view.hide();
  test.view.assertViewInactive();
};


/**
 * Test showing the pulsing animation and 'listening' text.
 */
test.view.testAudioDeviceReady = function() {
  view.show();
  view.setReadyForSpeech();
  assert(view.isVisible_);
  assertEquals(test.view.Text.LISTENING, test.view.interimText);
  assertEquals(test.view.Text.BLANK, test.view.finalText);
  assertEquals(test.view.Animation.INACTIVE, test.view.levelState);
};


/**
 * Test that the listening text is not shown and the animations are not
 * started if the UI hasn't been started.
 */
test.view.testAudioDeviceListeningBeforeViewStart = function() {
  view.setReadyForSpeech();
  test.view.assertViewInactive();
};


/**
 * Test the volume level animation after starting the view.
 */
test.view.testSpeechStartWithWorkingViews = function() {
  view.show();
  view.setReceivingSpeech();
  assert(view.isVisible_);
  assertEquals(test.view.Text.BLANK, test.view.interimText);
  assertEquals(test.view.Text.BLANK, test.view.finalText);
  assertEquals(test.view.Animation.ACTIVE, test.view.levelState);
  assertEquals(view.RECEIVING_SPEECH_CLASS_, view.container_.className);
};


/**
 * Test that the output text updates.
 */
test.view.testTextUpdateWithWorkingViews = function() {
  view.show();
  view.updateSpeechResult('interim', 'final');
  assert(view.isVisible_);
  assertEquals('interim', test.view.interimText);
  assertEquals('final', test.view.finalText);
  assertEquals(test.view.Animation.INACTIVE, test.view.levelState);
};


/**
 * Test that starting again after updating the output text doesn't change the
 * view state. This forces hide() to be called to restart the UI.
 */
test.view.testShowCalledAfterUpdate = function() {
  view.show();
  view.updateSpeechResult('interim', 'final');
  view.show();
  assert(view.isVisible_);
  assertEquals('interim', test.view.interimText);
  assertEquals('final', test.view.finalText);
  assertEquals(test.view.Animation.INACTIVE, test.view.levelState);
  assertEquals(view.OVERLAY_CLASS_, view.background_.className);
};


/**
 * Test the typical flow for the view.
 */
test.view.testTypicalFlowWithWorkingViews = function() {
  test.view.assertViewInactive();
  view.show();
  view.setReadyForSpeech();
  view.setReceivingSpeech();
  view.updateSpeechResult('interim1', 'final1');
  test.view.assertViewFullyActive(
      'interim1', 'final1', view.RECEIVING_SPEECH_CLASS_);
  view.updateSpeechResult('interim2', 'final2');
  test.view.assertViewFullyActive(
      'interim2', 'final2', view.RECEIVING_SPEECH_CLASS_);
  view.hide();
  test.view.assertViewInactive();
};


/**
 * Test hiding the UI after showing it.
 */
test.view.testStopAfterStart = function() {
  view.show();
  view.hide();
  test.view.assertViewInactive();
};


/**
 * Test hiding the UI after audio start.
 */
test.view.testHideAfterAudioStart = function() {
  view.show();
  view.setReadyForSpeech();
  view.hide();
  test.view.assertViewInactive();
};


/**
 * Test hiding the UI after showing speech results.
 */
test.view.testHideAfterSpeechTranscriptReceived = function() {
  view.show();
  view.setReadyForSpeech();
  view.setReceivingSpeech();
  view.updateSpeechResult('interim', 'final');
  view.hide();
  test.view.assertViewInactive();
};


/**
 * Test hiding the UI after speech start.
 */
test.view.testHideAfterSpeechStart = function() {
  view.show();
  view.setReadyForSpeech();
  view.setReceivingSpeech();
  view.hide();
  test.view.assertViewInactive();
};


/**
 * Test that clicking the microphone button when the 'Didn't get that' message
 * has been shown sends the correct parameters to the controller.
 */
test.view.testClickMicToRetryWithNoMatch = function() {
  view.show();
  view.isNoMatchShown_ = true;

  const fakeClickEvent = {target: {id: microphone.RED_BUTTON_ID}};
  view.onWindowClick_(fakeClickEvent);

  assert(test.view.state.shouldRetry);
  assert(!test.view.state.shouldSubmit);
};


/**
 * Test that clicking the retry link when the 'Didn't get that' message has been
 * shown sends the correct parameters to the controller.
 */
test.view.testClickLinkToRetryWithNoMatch = function() {
  view.show();
  view.isNoMatchShown_ = true;

  const fakeClickEvent = {target: {id: text.ERROR_LINK_ID}};
  view.onWindowClick_(fakeClickEvent);

  assert(test.view.state.shouldRetry);
  assert(!test.view.state.shouldSubmit);
};


/**
 * Test that clicking the microphone button when the 'Didn't get that' message
 * is not shown sends the correct parameters to the controller.
 */
test.view.testClickMicToRetryWithoutNoMatch = function() {
  view.show();

  const fakeClickEvent = {target: {id: microphone.RED_BUTTON_ID}};
  view.onWindowClick_(fakeClickEvent);

  assert(!test.view.state.shouldRetry);
  assert(test.view.state.shouldSubmit);
};


/**
 * Test that clicking the retry link when the 'Didn't get that' message is not
 * shown sends the correct shouldRetry parameter to the controller.
 */
test.view.testClickLinkToRetryWithoutNoMatch = function() {
  view.show();

  const fakeClickEvent = {target: {id: view.ERROR_LINK}};
  view.onWindowClick_(fakeClickEvent);

  assert(!test.view.state.shouldRetry);
  assert(!test.view.state.shouldSubmit);
};


// ***************************** HELPER FUNCTIONS *****************************
// Helper functions used in tests.


/**
 * Tests to make sure no components of the view are active.
 */
test.view.assertViewInactive = function() {
  assert(!view.isVisible_);
  assert(!view.isNoMatchShown_);
  assertEquals(test.view.Text.BLANK, test.view.interimText);
  assertEquals(test.view.Text.BLANK, test.view.finalText);
  assertEquals(test.view.Animation.INACTIVE, test.view.levelState);
  assertEquals(view.OVERLAY_HIDDEN_CLASS_, view.background_.className);
  assertEquals(view.INACTIVE_CLASS_, view.container_.className);
};


/**
 * Tests to make sure the all components of the view are working correctly.
 * @param {string} expectedInterimText The expected low confidence text string.
 * @param {string} expectedFinalText The expected high confidence text string.
 * @param {string} expectedContainerClass The expected CSS class of the
 *     container used to position the microphone and text output area.
 */
test.view.assertViewFullyActive = function(
    expectedInterimText, expectedFinalText, expectedContainerClass) {
  assert(view.isVisible_);
  assertEquals(expectedInterimText, test.view.interimText);
  assertEquals(expectedFinalText, test.view.finalText);
  assertEquals(test.view.Animation.ACTIVE, test.view.levelState);
  assertEquals(view.OVERLAY_CLASS_, view.background_.className);
  assertEquals(expectedContainerClass, view.container_.className);
};
