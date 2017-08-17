// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


onload = function() {
  var protectedDomain = "example.com";
  var unprotectedDomain = "a.com";
  var path = 'defaultresponse';

  runTests([
    // Navigates to an URL.
    function policySimpleLoad() {
      var protectedURL = getServerURL(path, protectedDomain);
      var unprotectedURL = getServerURL(path, unprotectedDomain);
      expect([
        { label: "a-onBeforeNavigate",
          event: "onBeforeNavigate",
          details: { frameId: 0,
                     parentFrameId: -1,
                     processId: -1,
                     tabId: 0,
                     timeStamp: 0,
                     url: unprotectedURL }},
        { label: "a-onCommitted",
          event: "onCommitted",
          details: { frameId: 0,
                     processId: 0,
                     tabId: 0,
                     timeStamp: 0,
                     transitionQualifiers: [],
                     transitionType: "link",
                     url: unprotectedURL }},
        { label: "a-onDOMContentLoaded",
          event: "onDOMContentLoaded",
          details: { frameId: 0,
                     processId: 0,
                     tabId: 0,
                     timeStamp: 0,
                     url: unprotectedURL }},
        { label: "a-onCompleted",
          event: "onCompleted",
          details: { frameId: 0,
                     processId: 0,
                     tabId: 0,
                     timeStamp: 0,
                     url: unprotectedURL }}],
        [ navigationOrder("a-") ]);
      navigateAndWait(protectedURL, function() {
        navigateAndWait(unprotectedURL,
            chrome.test.callbackPass())
      })
    }
  ]);
};
