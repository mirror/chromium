// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var testId = "bjafgdebaacbbbecmhlhpofkepfkgcpa";

// Call with |api| as either chrome.runtime or chrome.extension.
function connectExternalTest(api) {
  var port = api.connect(testId, {name: "extern"});
  port.postMessage({testConnectExternal: true});
  port.onMessage.addListener(chrome.test.callbackPass(function(msg) {
    chrome.test.assertTrue(msg.success, "Message failed.");
    chrome.test.assertEq(msg.senderId, location.host,
                         "Sender ID doesn't match.");
  }));
}

// Generates the list of test functions.
function generateTests() {
  let tests = [function connectExternal_runtime() {
    connectExternalTest(chrome.runtime);
  }];

  // In Chrome, also test chrome.extension, which is aliased to chrome.runtime.
  if (chrome.extension) {
    tests.push(function connectExternal_extension() {
      connectExternalTest(chrome.extension);
    });
  }

  return tests;
}

chrome.test.runTests(generateTests());
