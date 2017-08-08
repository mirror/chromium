// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Run ../update.sh whenever this file is changed.
 */

chrome.test.runTests([
  function virtualKeyboardAllRestrict() {
    chrome.virtualKeyboard.restrictFeatures(
        {
          autoCompleteEnabled: false,
          autoCorrectEnabled: false,
          spellCheckEnabled: false,
          voiceInputEnabled: false,
          handwritingEnabled: false
        },
        function() {
          chrome.virtualKeyboardPrivate.getKeyboardConfig(
              chrome.test.callbackPass(function(config) {
                chrome.test.assertTrue(
                    config.features.includes('autocomplete-disabled'));
                chrome.test.assertTrue(
                    config.features.includes('autocorrect-disabled'));
                chrome.test.assertTrue(
                    config.features.includes('spellcheck-disabled'));
                chrome.test.assertTrue(
                    config.features.includes('voiceinput-disabled'));
                chrome.test.assertTrue(
                    config.features.includes('handwriting-disabled'));
              }));
        });
  },
  function virtualKeyboardAllEnable() {
    chrome.virtualKeyboard.restrictFeatures(
        {
          autoCompleteEnabled: true,
          autoCorrectEnabled: true,
          spellCheckEnabled: true,
          voiceInputEnabled: true,
          handwritingEnabled: true
        },
        function() {
          chrome.virtualKeyboardPrivate.getKeyboardConfig(
              chrome.test.callbackPass(function(config) {
                chrome.test.assertTrue(
                    config.features.includes('autocomplete-enabled'));
                chrome.test.assertTrue(
                    config.features.includes('autocorrect-enabled'));
                chrome.test.assertTrue(
                    config.features.includes('spellcheck-enabled'));
                chrome.test.assertTrue(
                    config.features.includes('voiceinput-enabled'));
                chrome.test.assertTrue(
                    config.features.includes('handwriting-enabled'));
              }));
        });
  },
  function virtualKeyboardPartialRectrict() {
    chrome.virtualKeyboard.restrictFeatures(
        {
          autoCompleteEnabled: false,
          autoCorrectEnabled: true,
          spellCheckEnabled: false,
          voiceInputEnabled: true,
          // handwritingEnabled is omitted; the current config should be used.
        },
        chrome.test.callbackPass(function() {
          chrome.virtualKeyboardPrivate.getKeyboardConfig(
              chrome.test.callbackPass(function(config) {
                chrome.test.assertTrue(
                    config.features.includes('autocomplete-disabled'));
                chrome.test.assertTrue(
                    config.features.includes('autocorrect-enabled'));
                chrome.test.assertTrue(
                    config.features.includes('spellcheck-disabled'));
                chrome.test.assertTrue(
                    config.features.includes('voiceinput-enabled'));
                chrome.test.assertTrue(
                    config.features.includes('handwriting-enabled'));
              }));
        }));
  },
  function virtualKeyboardOnConfigChanged() {
    chrome.virtualKeyboardPrivate.onKeyboardConfigChanged.addListener(
        // The callback parameter should be the same as the return value of
        // |getKeyboardConfig|
        chrome.test.callbackPass(function(config) {
          chrome.test.assertTrue(!!config);
          chrome.virtualKeyboardPrivate.getKeyboardConfig(
              chrome.test.callbackPass(function(config2) {
                chrome.test.assertEq(config, config2);
              }));
        }));

    chrome.virtualKeyboard.restrictFeatures(
        {
          autoCompleteEnabled: true,
          autoCorrectEnabled: true,
          spellCheckEnabled: false,
          voiceInputEnabled: true,
          handwritingEnabled: false
        },
        chrome.test.callbackPass(function() {}));
  },
]);
