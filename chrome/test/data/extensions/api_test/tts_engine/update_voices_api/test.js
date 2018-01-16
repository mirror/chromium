// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var testVoiceData = [
  {
    voiceName: 'David',
    lang: 'zh-TW',
    gender: 'male',
    eventTypes: ['start']
  },
  {
    voiceName: 'Laura',
    lang: 'en-GB',
    gender: 'female',
    eventTypes: ['end', 'interrupted', 'cancelled']
  }
];

function setup() {
  var speakListener = function(utterance, options, sendTtsEvent) {};
  var stopListener = function() {};
  chrome.ttsEngine.onSpeak.addListener(speakListener);
  chrome.ttsEngine.onStop.addListener(stopListener);
}

function assertVoice(expectedVoice, actualVoice) {
  for (var key in expectedVoice) {
    chrome.test.assertEq(expectedVoice[key], actualVoice[key]);
  }
}

chrome.test.runTests([
  function testGetVoices() {
    setup();
    chrome.tts.getVoices(function(voices) {
      chrome.test.assertEq(1, voices.length);
      assertVoice({
        voiceName: 'Zach',
        lang: 'en-US',
        eventTypes: [ 'end', 'interrupted', 'cancelled', 'error']
      }, voices[0]);
      chrome.ttsEngine.updateVoices(testVoiceData);
      chrome.tts.getVoices(function(runtimeVoices) {
        chrome.test.assertEq(testVoiceData.length, runtimeVoices.length);
        for (var i = 0; i < runtimeVoices.length; i++)
          assertVoice(testVoiceData[i], runtimeVoices[i]);
        chrome.test.assertNoLastError();
        chrome.test.succeed();
      });
    });
  }
]);
