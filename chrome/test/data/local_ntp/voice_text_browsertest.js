// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


/**
 * @fileoverview Tests the text module of Voice Search on the local NTP.
 */


// ******************************* SIMPLE TESTS *******************************
// These are run by runSimpleTests above.
// Functions from test_utils.js are automatically imported.


let clock;


let stubs_;


function setUp() {
  setUpPage('voice-text-template');
  clock = new MockClock();
  stubs_ = new Replacer();

  clock.install_();
  stubs_.replace(window, 'getChromeUILanguage', () => 'en-ZA');
  stubs_.replace(speech, 'messages', {
    audioError: 'Audio error',
    details: 'Details',
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
  });

  text.init();
}


function tearDown() {
  text.interim_.textContent = '';
  text.final_.textContent = '';
  clock.dispose();
  stubs_.reset();
}


/**
 * Makes sure text sets up with the correct settings.
 */
function testInit() {
  assertEquals('', text.interim_.textContent);
  assertEquals('', text.final_.textContent);
}


/**
 * Test updating the text values.
 */
function testUpdateText() {
  const interimText = 'interim';
  const finalText = 'final';
  text.updateTextArea(interimText, finalText);
  assertEquals(interimText, text.interim_.textContent);
  assertEquals(finalText, text.final_.textContent);
}


/**
 * Test updating the text with an error message containing a link.
 */
function testShowErrorMessageWithLink() {
  const noAudioError = RecognitionError.AUDIO_CAPTURE;
  text.showErrorMessage(noAudioError);
  assertEquals(
      'Audio error <a id="voice-text-area" ' +
          `href="${text.SUPPORT_LINK_BASE_.replace(/&/, '&amp;')}en-ZA" ` +
          'target="_blank">Learn more</a>',
      text.interim_.innerHTML);
  assertEquals('', text.final_.innerHTML);
}


/**
 * Test updating the text with an error message containing a callback.
 */
function testShowErrorMessageWithCallback() {
  const tryAgainError = RecognitionError.NO_MATCH;
  text.showErrorMessage(tryAgainError);
  assertEquals(
      'No translation <a id="voice-text-area">Try again</a>',
      text.interim_.innerHTML);
  assertEquals('', text.final_.innerHTML);
}


/**
 * Test clearing the text values.
 */
function testClearText() {
  const interimText = 'interim';
  const finalText = 'final';
  text.interim_.textContent = interimText;
  text.final_.textContent = finalText;
  text.clear();
  assertEquals('voice-text', text.interim_.className);
  assertEquals('voice-text', text.final_.className);
}


/**
 * Test starting initialization message.
 */
function testSetInitializationMessage() {
  text.interim_.textContent = 'interim text';
  text.final_.textContent = 'final text';

  clock.setTime(1);
  text.showInitializingMessage();

  assertEquals(1, clock.pendingTimeouts.length);
  assertEquals(301, clock.pendingTimeouts[0].activationTime);

  clock.advanceTime(300);
  clock.pendingTimeouts.shift().callback();

  assertEquals(speech.messages.waiting, text.interim_.textContent);
  assertEquals('', text.final_.textContent);
  assertEquals(0, clock.pendingTimeouts.length);
}


/**
 * Test showing the ready message.
 */
function testReadyMessage() {
  text.interim_.textContent = 'interim text';
  text.final_.textContent = 'final text';

  clock.setTime(1);
  text.showReadyMessage();

  assertEquals(speech.messages.ready, text.interim_.textContent);
  assertEquals('', text.final_.textContent);
  assertEquals(1, clock.pendingTimeouts.length);
  assertEquals(2001, clock.pendingTimeouts[0].activationTime);
}


/**
 * Test the listening message animation when the ready message is shown.
 */
function testListeningAnimationWhenReady() {
  text.interim_.textContent = speech.messages.ready;

  clock.setTime(1);
  text.startListeningMessageAnimation_();

  assertEquals(1, clock.pendingTimeouts.length);
  assertEquals(2001, clock.pendingTimeouts[0].activationTime);

  clock.advanceTime(2000);
  clock.pendingTimeouts.shift().callback();

  assertEquals(speech.messages.listening, text.interim_.textContent);
  assertEquals('', text.final_.textContent);
  assertEquals(0, clock.pendingTimeouts.length);
}


/**
 * Test the listening message animation when the ready message is not shown.
 */
function testListeningAnimationWhenNotReady() {
  text.interim_.textContent = 'some text';

  clock.setTime(1);
  text.startListeningMessageAnimation_();

  assertEquals(1, clock.pendingTimeouts.length);
  assertEquals(2001, clock.pendingTimeouts[0].activationTime);

  clock.advanceTime(2000);
  clock.pendingTimeouts.shift().callback();

  assertEquals('some text', text.interim_.textContent);
  assertEquals('', text.final_.textContent);
  assertEquals(0, clock.pendingTimeouts.length);
}


/**
 * Test the listening message animation interruption.
 */
function testListeningAnimationInterruption() {
  text.interim_.textContent = speech.messages.ready;

  clock.setTime(1);
  text.startListeningMessageAnimation_();

  assertEquals(1, clock.pendingTimeouts.length);
  assertEquals(2001, clock.pendingTimeouts[0].activationTime);

  clock.advanceTime(2000);
  clock.pendingTimeouts.shift().callback();

  assertEquals(speech.messages.listening, text.interim_.textContent);
  assertEquals('', text.final_.textContent);
  assertEquals(0, clock.pendingTimeouts.length);

  clock.advanceTime(30);
  text.interim_.textContent = 'New message!';

  assertEquals('New message!', text.interim_.textContent);
  assertEquals('', text.final_.textContent);
  assertEquals(0, clock.pendingTimeouts.length);
}
