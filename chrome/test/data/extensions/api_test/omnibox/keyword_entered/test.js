// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function registerOnInputChangedListener() {
  chrome.omnibox.onInputChanged.addListener(
    function(text, suggest) {
      chrome.test.log("onInputChanged: " + text);
      suggest([{content: text + " 1", description: "description"}]);
  });

  var error = !!chrome.runtime.lastError;
  domAutomationController.send(error);
}

function registerOnInputStartedListener() {
  chrome.omnibox.onInputStarted.addListener(
    function() {
      chrome.test.sendMessage("onInputStarted");
  });

  var error = !!chrome.runtime.lastError;
  domAutomationController.send(error);
}

function registerOnKeywordEnteredListener() {
  chrome.omnibox.onKeywordEntered.addListener(
    function(suggest) {
      var resultText = "entered ";
      suggest([
        {content: resultText + "1", description: resultText + "1"},
        {content: resultText + "2", description: resultText + "2"}
      ]);
      chrome.test.sendMessage("onKeywordEntered");
  });

  var error = !!chrome.runtime.lastError;
  domAutomationController.send(error);
}
