// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Run ../update.sh whenever this file is changed.
 */

chrome.test.runTests([
  function virtualKeyboardExists() {
    chrome.test.assertTrue(!!chrome.virtualKeyboard);
    chrome.test.callbackPass(function() {})();
  },
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


                //        chrome.test.assertFalse(config.features.autoComplete);
                //        chrome.test.assertFalse(config.features.autoCorrect);
                //        chrome.test.assertFalse(config.features.spellCheck);
                //        chrome.test.assertFalse(config.features.voiceInputEnabled);
                //        chrome.test.assertFalse(config.features.handwriting);
              }));
        });
  },
]);
