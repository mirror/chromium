// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * The packed extension resides in
 * ../webstore/downloads/lgdbgjkfohfihknakdlipddibehgbdmb.crx .
 * Update it too whenever this file is updated.
 * See https://developer.chrome.com/extensions/packaging#packaging for how to
 * package.
 */

chrome.test.runTests([
  function enableVoiceInputShouldPopulateError() {
    chrome.virtualKeyboard.restrictFeatures(
        {
          voiceInputEnabled: true,
        },
        chrome.test.callbackFail(
            'Voice input enabled but the device has no audio input.'));
  },
]);
