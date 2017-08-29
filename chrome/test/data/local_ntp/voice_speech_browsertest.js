// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


/**
 * @fileoverview Tests the speech module of Voice Search on the local NTP.
 */


const TEST_BASE_URL_ = 'https://google.com/';

/**
 * A module configuration for the test.
 */
const TEST_STRINGS_ = {
  audioError: 'Audio error',
  details: 'Details',
  fakeboxMicrophoneTooltip: 'fakebox-speech tooltip',
  languageError: 'Language error',
  learnMore: 'Learn more',
  listening: 'Listening',
  networkError: 'Network error',
  noTranslation: 'No translation',
  noVoice: 'No voice',
  permissionError: 'Permission error',
  ready: 'Ready',
  tryAgain: 'Try again',
  waiting: 'Waiting'
};


const TEST_IDS_ = {
  FAKEBOX_SPEECH: 'fakebox-speech'
};


/**
 * Mock out the clock function for testing timers.
 */
let clock;


/**
 * Specifies the number of alternative suggestions to return in the final
 * result.
 * @type {number}
 */
let numAlternatives;


/**
 * Keeps track of the number of view activations.
 * @type {number}
 */
let viewActiveCount;


/**
 * Keeps track of the number of speech recognition activations.
 * @type {number}
 */
let recognitionActiveCount;


/**
 * Stores the recorded log messages.
 * @type {Array<string>}
 */
let logMessages;


/**
 * Stores the last message posted to the window.
 * @type {string}
 */
let lastWindowMessage;


/**
 * Keeps the current state.
 * @type {Object}
 */
let state;


/**
 * Utility to mock out parts of the DOM.
 */
let stubs_;


function setUp() {
  setUpPage('voice-speech-template');
  state = {};
  stubs_ = new Replacer();

  stubs_.replace(window, 'getChromeUILanguage', () => 'en-ZA');
  stubs_.replace(view, 'init', function() {
    return true;
  });
  stubs_.replace(view, 'updateSpeechResult', setStateVariable('viewText'));
  stubs_.replace(view, 'setReadyForSpeech', setStateVariable('viewReady'));
  stubs_.replace(view, 'showError', setStateVariable('viewError'));
  stubs_.replace(view, 'setReceivingSpeech', setStateVariable('viewReceiving'));
  stubs_.replace(view, 'hide', function() {
    viewActiveCount--;
  });
  stubs_.replace(view, 'show', function() {
    viewActiveCount++;
  });
  stubs_.replace(window, 'postMessage', function(message, _) {
    lastWindowMessage = message.type;
  });

  stubs_.replace(window, 'postMessage', function(message, _) {
    lastWindowMessage = message.type;
  });

  const handleSpeechRecognitionEnd = function(args) {
    state['query'] = args[0];
    state['searchMethod'] = args[1];
  };
  const handleSpeechRecognitionResult = function(args) {
    state['query'] = args[0];
  };

  const handleSpeechRecognitionDisabled = function(args) {
    state['currentlyDisabled'] = true;
  };

  numAlternatives = 4;
  setWebkitSpeechRecognition();
  logMessages = [];
  recognitionActiveCount = 0;
  viewActiveCount = 0;

  clock = new MockClock();
}

function initSpeech_() {
  speech.init(
      TEST_BASE_URL_, TEST_STRINGS_, $(TEST_IDS_.FAKEBOX_SPEECH),
      window.chrome.embeddedSearch.searchBox);
}


function tearDown() {
  lastWindowMessage = undefined;

  // Uninitialize speech between tests to allow message listeners to reset.
  speech.uninit_(
      $(TEST_IDS_.FAKEBOX_SPEECH), window.chrome.embeddedSearch.searchBox);
  stubs_.reset();
  clock.dispose();
}


// ******************************* SIMPLE TESTS *******************************
// These are run by runSimpleTests from test_utils.js.
// Functions from test_utils are automatically imported.


/**
 * Tests if the controller has the correct webkit speech recognition settings.
 */
function testWebkitSpeechRecognitionInitSettings() {
  initSpeech_();
  assert(!speech.recognition_.continuous);
  assertEquals('en-ZA', speech.recognition_.lang);
  assertEquals(4, speech.recognition_.maxAlternatives);
  assert(speech.isRecognitionInitialized_());
  validateInactive();
}


/**
 * Test that the click listeners are only added once between JESR transitions.
 */
function testInitSuccessfullyChangesState() {
  assertEquals(speech.State_.UNINITIALIZED, speech.currentState_);
  initSpeech_();
  assertEquals(speech.State_.READY, speech.currentState_);
  assertThrows('not in UNINITIALIZED', () => initSpeech_());
}


/**
 * Tests that with everything OK, sending a button click message from the
 * embedded search page initiates speech and sending an omnibox focused message
 * terminates speech.
 */
function testNtpClickHandlingAndOmniboxFocusWithWorkingView() {
  initSpeech_();
  $(TEST_IDS_.FAKEBOX_SPEECH).click();

  assertEquals(1, recognitionActiveCount);
  assertEquals(1, viewActiveCount);
  assert(speech.isRecognizing_());
  assert(clock.isTimeoutSet(speech.idleTimer_));
  assert(!clock.isTimeoutSet(speech.errorTimer_));

  speech.onOmniboxFocused();
  validateInactive();
}


/**
 * Tests that with everything OK, sending a button click message from the
 * searchbox initiates speech.
 */
function testSearchboxClickHandlingWithWorkingView() {
  initSpeech_();
  $(TEST_IDS_.FAKEBOX_SPEECH).click();

  assertEquals(1, recognitionActiveCount);
  assertEquals(1, viewActiveCount);
  assert(speech.isRecognizing_());
  assert(clock.isTimeoutSet(speech.idleTimer_));
  assert(!clock.isTimeoutSet(speech.errorTimer_));
}


/**
 * Tests that when the webkit recognition interface is uninitialized,
 * clicking the speech input tool initializes it prior to starting the
 * interface.
 */
function testClickHandlingWithUnitializedWebkitRecognition() {
  initSpeech_();
  speech.recognition_ = false;
  assert(speech.currentState_ !== speech.State_.UNINITIALIZED);
  assert(!speech.isRecognitionInitialized_());
  speech.toggleStartStop();

  assertEquals(1, recognitionActiveCount);
  assertEquals(1, viewActiveCount);
  assert(speech.isRecognitionInitialized_());
}


/**
 * Tests that when the webkit recognition interface is uninitialized,
 * clicking the speech input tool initializes it prior to starting the
 * interface.
 */
function testBrokenWebkitSpeechRecognition() {
  webkitSpeechRecognition = null;
  initSpeech_();

  assert(state['currentlyDisabled']);

  assert($(TEST_IDS_.FAKEBOX_SPEECH));
  $(TEST_IDS_.FAKEBOX_SPEECH).click();
  validateInactive();

  const keyEvent = new KeyboardEvent(
      'testEvent', {'code': 'Period', 'shiftKey': true, 'ctrlKey': true});
  speech.onKeyDown(keyEvent);
  validateInactive();

  // Restore webkitSpeechRecognition.
  setWebkitSpeechRecognition();
}


/**
 * Tests that the view is notified when the webkit speech recognition interface
 * starts the audio driver.
 */
function testHandleAudioStart() {
  initSpeech_();
  speech.toggleStartStop();
  speech.recognition_.onaudiostart();

  assert('viewReady' in state);
  assert(speech.isRecognizing_());
  assertEquals(1, recognitionActiveCount);
}


/**
 * Tests that the suggestbox is hidden when the webkit speech recognition
 * interface detects speech input.
 */
function testHandleSpeechStart() {
  initSpeech_();
  speech.toggleStartStop();
  speech.recognition_.onaudiostart(null);
  speech.recognition_.onspeechstart(null);

  assert('viewReceiving' in state);
  assert(!elementIsVisible($(TEST_IDS_.FAKEBOX_SPEECH)));
}


/**
 * Tests the handling of a response received from the webkit speech recognition
 * interface while the user is still speaking.
 */
function testHandleInterimSpeechResponse() {
  initSpeech_();
  const lowConfidenceText = 'low';
  const highConfidenceText = 'high';
  const expectedQuery = highConfidenceText + lowConfidenceText;
  const responseEvent =
      getInterimResponse(lowConfidenceText, highConfidenceText);

  speech.toggleStartStop();
  speech.recognition_.onaudiostart(null);
  speech.recognition_.onspeechstart(null);
  speech.recognition_.onresult(responseEvent);

  assert(speech.isRecognizing_());
  validateStateVariable('viewText', [expectedQuery, highConfidenceText]);
  validateStateVariable('query', expectedQuery);
}


/**
 * Tests the handling of a response received from the webkit speech recognition
 * interface after the user has finished speaking.
 */
function testHandleFinalSpeechResponse() {
  initSpeech_();
  const lowConfidenceText = 'low';
  const highConfidenceText = 'high';
  const query = highConfidenceText;
  const responseEvent = getFinalResponse(lowConfidenceText, highConfidenceText);

  speech.toggleStartStop();
  speech.recognition_.onaudiostart(null);
  speech.recognition_.onspeechstart(null);
  speech.recognition_.onresult(responseEvent);

  validateStateVariable('query', query);
  validateStateVariable('searchMethod', speech.SearchMethod_.FINAL_RESULT);
  validateInactive();
}


/**
 * Tests the handling of user-interrupted speech recognition after an interim
 * speech recognition result is received.
 */
function testInterruptSpeechInputAfterInterimResult() {
  initSpeech_();
  const lowConfidenceText = 'low';
  const highConfidenceText = 'high';
  const responseEvent =
      getInterimResponse(lowConfidenceText, highConfidenceText);

  speech.toggleStartStop();
  speech.recognition_.onaudiostart(null);
  speech.recognition_.onspeechstart(null);
  speech.recognition_.onresult(responseEvent);
  speech.toggleStartStop();

  assert(!speech.isRecognizing_());
  assertEquals('', speech.interimResult_);
  assertEquals('', speech.finalResult_);
  assertEquals('', speech.speechQuery_);
  assertEquals(0, recognitionActiveCount);

  getInterimResponse('result should', 'be ignored');
  speech.recognition_.onresult(responseEvent);

  assert(!speech.isRecognizing_());
  assertEquals('', speech.interimResult_);
  assertEquals('', speech.finalResult_);
  assertEquals('', speech.speechQuery_);
  assertEquals(0, recognitionActiveCount);
}


/**
 * Tests the handling of user-interrupted speech recognition before any result
 * is received..
 */
function testInterruptSpeechInputBeforeResult() {
  initSpeech_();
  speech.toggleStartStop();
  speech.recognition_.onaudiostart(null);
  speech.recognition_.onspeechstart(null);
  speech.toggleStartStop();

  validateInactive();
  assertEquals(1, logMessages.length);
  assertEquals('0&reason=5', logMessages[0]);
}


/**
 * Tests webkit speech recognition error.
 */
function testWebkitSpeech2Error() {
  initSpeech_();
  speech.toggleStartStop();
  speech.recognition_.onerror({error: 'some-error'});

  assert(!speech.isRecognizing_());
  assertEquals(1, recognitionActiveCount);
  assertEquals(1, viewActiveCount);
  assertEquals(1, logMessages.length);
  validateStateVariable('viewError', speech.RecognitionError.OTHER);
  assertEquals('2&reason=9&data=some-error', logMessages[0]);
  clock.advanceTime(2999);
  assertEquals(1, viewActiveCount);

  clock.advanceTime(1);
  validateInactive();
}


/**
 * Tests no speech input received.
 */
function testNoSpeechInput() {
  initSpeech_();
  speech.toggleStartStop();
  speech.recognition_.onaudiostart(null);
  speech.recognition_.onend(null);

  assert(!speech.isRecognizing_());
  validateStateVariable('viewError', speech.RecognitionError.NO_SPEECH);
  assertEquals(1, logMessages.length);
  assertEquals('3&reason=0', logMessages[0]);

  clock.advanceTime(7999);
  assertEquals(1, viewActiveCount);

  clock.advanceTime(1);
  validateInactive();
}


/**
 * Tests enter to submit speech query.
 */
function testEnterToSubmit() {
  initSpeech_();
  const query = 'test query';
  const keyEvent = new KeyboardEvent('testEvent', {'code': 'Enter'});

  speech.toggleStartStop();
  speech.recognition_.onaudiostart(null);
  speech.recognition_.onspeechstart(null);
  speech.speechQuery_ = query;

  speech.onKeyDown(keyEvent);

  validateInactive();
  validateStateVariable('query', query);
  validateStateVariable('searchMethod', speech.SearchMethod_.ENTER_KEY);
}


/**
 * Tests click to submit.
 */
function testClickToSubmit() {
  initSpeech_();
  speech.toggleStartStop();
  speech.recognition_.onaudiostart(null);
  speech.recognition_.onspeechstart(null);
  const query = 'test query';
  speech.speechQuery_ = query;
  speech.onClick_(true, false);

  validateInactive();
  validateStateVariable('query', query);
  validateStateVariable('searchMethod', speech.SearchMethod_.CLICK);
}


/**
 * Tests speech recognition initiated with <CTRL> + <SHIFT> + <.>.
 */
function testKeyboardStartWithCtrl() {
  initSpeech_();

  const keyEvent = new KeyboardEvent('testEvent', {'code': 'Period'});
  speech.onKeyDown(keyEvent);
  validateInactive();

  keyEvent.shiftKey = true;
  speech.onKeyDown(keyEvent);
  validateInactive();

  keyEvent.ctrlKey = true;
  speech.onKeyDown(keyEvent);
  assert(speech.isRecognizing_());

  speech.onKeyDown(keyEvent);
  assert(speech.isRecognizing_());
}


/**
 * Tests speech recognition initiated with <CMD> + <SHIFT> + <.> on mac.
 */
function testKeyboardStartWithCmd() {
  initSpeech_();

  const keyEvent = new KeyboardEvent('testEvent', {'code': 'Period'});
  speech.onKeyDown(keyEvent);
  validateInactive();

  keyEvent.shiftKey = true;
  speech.onKeyDown(keyEvent);
  validateInactive();

  // Save the user agent state.
  const userAgentOld = window.navigator.userAgent;

  // Set a non-mac user agent.
  window.navigator.userAgent = 'Mozilla/5.0 (Windows NT 6.1) AppleWebKit/537' +
      '.36 (KHTML, like Gecko) Chrome/41.0.2228.0 Safari/537.36';
  keyEvent.metaKey = true;
  speech.onKeyDown(keyEvent);
  validateInactive();

  // Set a mac user agent.
  window.navigator.userAgent =
      'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_1) AppleWebKit/537' +
      '.36 (KHTML, like Gecko) Chrome/41.0.2227.1 Safari/537.36';
  speech.onKeyDown(keyEvent);
  assert(speech.isRecognizing_());

  speech.onKeyDown(keyEvent);
  assert(speech.isRecognizing_());

  // Restore the user agent state.
  window.navigator.userAgent = userAgentOld;
}


/**
 * Tests click to abort.
 */
function testClickToAbort() {
  initSpeech_();
  speech.toggleStartStop();
  speech.recognition_.onaudiostart(null);
  speech.recognition_.onspeechstart(null);
  speech.onClick_(false, false);

  validateInactive();
  assertEquals(1, logMessages.length);
  assertEquals('1&reason=abort&data=more_info', logMessages[0]);
}


/**
 * Tests click to retry when the interface is stopped.
 */
function testClickToRetryWhenStopped() {
  initSpeech_();
  speech.toggleStartStop();
  speech.recognition_.onaudiostart(null);
  speech.recognition_.onspeechstart(null);
  // An onend event after onpeechstart forces an error and stops recognition.
  speech.recognition_.onend(null);
  speech.onClick_(false, true);

  assert(speech.isRecognizing_());
  assertEquals(2, logMessages.length);
  assertEquals('3&reason=8', logMessages[0]);  // Forced error for early onend.
  assertEquals('6&reason=click&data=retry', logMessages[1]);
}


/**
 * Tests click to retry when the interface is not stopped.
 */
function testClickToRetryWhenNotStopped() {
  initSpeech_();
  speech.toggleStartStop();
  speech.recognition_.onaudiostart(null);
  speech.recognition_.onspeechstart(null);
  speech.onClick_(false, true);

  validateInactive();
  assertEquals(1, logMessages.length);
  assertEquals('1&reason=click&data=retry', logMessages[0]);
}


/**
 * Tests for speech with no matches.
 */
function testNoSpeechInputMatched() {
  initSpeech_();
  speech.toggleStartStop();
  speech.recognition_.onaudiostart(null);
  speech.recognition_.onspeechstart(null);
  speech.recognition_.onnomatch(null);

  assert(!speech.isRecognizing_());
  assertEquals(1, recognitionActiveCount);
  assertEquals(1, viewActiveCount);
  validateStateVariable('viewError', speech.RecognitionError.NO_MATCH);
  assertEquals(1, logMessages.length);
  assertEquals('4', logMessages[0]);
  clock.advanceTime(7999);
  assertEquals(1, viewActiveCount);

  clock.advanceTime(1);
  validateInactive();
}


/**
 * Tests that if the browser doesn't get permission to use the microphone the
 * permission bar decorator is shown.
 */
function testPermissionTimeout() {
  initSpeech_();
  speech.toggleStartStop();

  clock.advanceTime(1499);
  assert(!('waitingForPermission' in state));

  clock.advanceTime(1);
  assert('waitingForPermission' in state);
  assertEquals(1, recognitionActiveCount);
  assertEquals(1, viewActiveCount);
  assertEquals(1, logMessages.length);
  assertEquals('0&reason=6', logMessages[0]);
  assert(clock.isTimeoutSet(speech.idleTimer_));
  assert(!clock.isTimeoutSet(speech.errorTimer_));

  clock.advanceTime(14999);
  assertEquals(1, recognitionActiveCount);
  assertEquals(1, viewActiveCount);

  clock.advanceTime(1);
  validateInactive();
  assertEquals(2, logMessages.length);
  assertEquals('0&reason=3', logMessages[1]);
}


/**
 * Tests that if no interactions occur after after some time during speech
 * recognition, the current final speech results are submitted for search.
 */
function testIdleTimeoutWithFinalSpeechResults() {
  initSpeech_();
  const lowConfidenceText = 'low';
  const highConfidenceText = 'high';
  const responseEvent =
      getInterimResponse(lowConfidenceText, highConfidenceText);
  speech.reset_();

  speech.toggleStartStop();
  speech.recognition_.onaudiostart(null);
  speech.recognition_.onspeechstart(null);
  speech.recognition_.onresult(responseEvent);

  clock.advanceTime(7999);
  assertEquals(1, recognitionActiveCount);
  assertEquals(1, viewActiveCount);

  clock.advanceTime(1);
  validateInactive();
  assertEquals(0, logMessages.length);
  validateStateVariable('query', highConfidenceText);
  validateStateVariable('searchMethod', speech.SearchMethod_.IDLE_TIMEOUT);
}


/**
 * Tests that if no interactions occur after after some time during speech
 * recognition before final speech results are received, that the interface
 * closes.
 */
function testIdleTimeoutWithNoFinalSpeechResults() {
  initSpeech_();
  const lowConfidenceText = 'low';
  const highConfidenceText = '';
  const responseEvent =
      getInterimResponse(lowConfidenceText, highConfidenceText);
  speech.reset_();

  speech.toggleStartStop();
  speech.recognition_.onaudiostart(null);
  speech.recognition_.onspeechstart(null);
  speech.recognition_.onresult(responseEvent);

  clock.advanceTime(7999);
  assertEquals(1, recognitionActiveCount);
  assertEquals(1, viewActiveCount);

  clock.advanceTime(1);
  validateInactive();
  assertEquals(1, logMessages.length);
  assertEquals('0&reason=3', logMessages[0]);
}


// ***************************** HELPER FUNCTIONS *****************************
// These are used by the simple tests above.


/**
 * Sets the {@code state} variable to the specified value.
 * @param {string} name The name of the variable to be set.
 * @param {Object} value The value to set the variable to.
 * @return {Function} A function that sets the the state variable.
 */
function setStateVariable(name, value) {
  return function() {
    if (value === undefined) {
      if (arguments.length == 1) {
        state[name] = arguments[0];
      } else {
        state[name] = Array.prototype.slice.call(arguments);
      }
    } else {
      state[name] = value;
    }
  };
}


/**
 * Validates that the {@code state} variable has the expected values.
 * @param {string} name The name of the variable to be validated.
 * @param {Object} value The expected value of the state variable.
 */
function validateStateVariable(name, value) {
  assert(name in state);
  if (value instanceof Array) {
    assert(state[name] instanceof Array);
    assertEquals(value.length, state[name].length);
    for (let i = 0; i < value.length; ++i) {
      assertEquals(value[i], state[name][i]);
    }
  } else {
    assertEquals(value, state[name]);
  }
}


/**
 * Validates that Speech is currently inactive and ready to start recognition.
 */
function validateInactive() {
  assert(!speech.isRecognizing_());
  assertEquals(0, recognitionActiveCount);
  assertEquals(0, viewActiveCount);
  assert(!clock.isTimeoutSet(speech.idleTimer_));
  assert(!clock.isTimeoutSet(speech.errorTimer_));
}


/**
 * Generates a webkitSpeechRecognitionResponse corresponding to one that is
 * generated after the speech recognition server has detected the end of voice
 * input and generated a final transcription.
 * @param {string} interimText Low confidence transcript.
 * @param {string} finalText High confidence transcript.
 * @return {webkitSpeechRecognitionEvent} The response event.
 */
function getFinalResponse(interimText, finalText) {
  const response = getInterimResponse(interimText, finalText);
  response.results[response.resultIndex].isFinal = true;
  for (let j = 1; j < numAlternatives; ++j) {
    response.results[response.resultIndex][j] = 'suggestion ' + j;
  }
  return response;
}


/**
 * Generates a webkitSpeechRecognitionResponse corresponding to one that is
 * generated after the speech recognition server has generated a transcription
 * of the received input but the user hasn't finished speaking.
 * @param {string} interimText Low confidence transcript.
 * @param {string} finalText High confidence transcript.
 * @return {webkitSpeechRecognitionEvent} The response event.
 */
function getInterimResponse(interimText, finalText) {
  const response = new webkitSpeechRecognitionEvent();
  response.resultIndex = 0;
  response.results = [];
  response.results[0] = new webkitSpeechRecognitionResult();
  response.results[0][0] =
      getWebkitSpeechRecognitionAlternative(finalText, 0.9);
  response.results[1] = new webkitSpeechRecognitionResult();
  response.results[1][0] =
      getWebkitSpeechRecognitionAlternative(interimText, 0.1);
  return response;
}


/**
 * Generates a webkitSpeechRecognitionAlternative that stores a speech
 * transcription and confidence level.
 * @param {string} text The transcription text.
 * @param {confidence} confidence The confidence level of the transcript.
 * @return {webkitSpeechRecognitionResultAlternative} The generated speech
 * recognition transcription.
 */
function getWebkitSpeechRecognitionAlternative(text, confidence) {
  const alt = new webkitSpeechRecognitionResultAlternative();
  alt.transcript = text;
  alt.confidence = confidence;
  return alt;
}



/**
 * Mock of the webkitSpeechRecognition interface with just the parameters used
 * by the speech.
 * @constructor
 */
function setWebkitSpeechRecognition() {
  webkitSpeechRecognition = function() {
    this.start = function() {
      recognitionActiveCount++;
    };
    this.stop = function() {
      recognitionActiveCount--;
    };
    this.abort = function() {
      recognitionActiveCount--;
      // Hack to work around the fact that we have to abort twice
      // to ensure that the favicon stop flashing. Can be removed
      // once http://crbug/169528 is fixed.
      // TODO(oskopek): Verify and remove.
      if (recognitionActiveCount < 0) {
        recognitionActiveCount = 0;
      }
    };
    this.continuous = false;
    this.interimResults = false;
    this.maxAlternatives = 0;
    this.onerror = null;
    this.onnomatch = null;
    this.onend = null;
    this.onresult = null;
    this.onaudiostart = null;
    this.onspeechstart = null;
  };
}



/**
 * Mock of the webkitSpeechRecognitionEvent that is returned by the speech
 * recognition server when it translates speech input.
 * @constructor
 */
function webkitSpeechRecognitionEvent() {
  this.results = null;
  this.resultIndex = -1;
}



/**
 * Mock of the webkitSpeechRecognitionResult that stores
 * webkitSpeechRecognitionAlternative's and the boolean indicate whether this is
 * a final result or an interim result.
 * @constructor
 */
function webkitSpeechRecognitionResult() {
  this.isFinal = false;
}



/**
 * Mock of the webkitSpeechRecognitionAlternative that stores the server
 * generated speech transcripts.
 * @constructor
 */
function webkitSpeechRecognitionResultAlternative() {
  this.transcript = '';
  this.confidence = -1;
}
