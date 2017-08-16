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
      expect([
        { label: "a-onBeforeNavigate",
          event: "onBeforeNavigate",
          details: { frameId: 0,
                     parentFrameId: -1,
                     processId: -1,
                     tabId: 0,
                     timeStamp: 0,
                     url: getServerURL(path, unprotectedDomain) }},
        { label: "a-onCommitted",
          event: "onCommitted",
          details: { frameId: 0,
                     processId: 0,
                     tabId: 0,
                     timeStamp: 0,
                     transitionQualifiers: [],
                     transitionType: "link",
                     url: getServerURL(path, unprotectedDomain) }},
        { label: "a-onDOMContentLoaded",
          event: "onDOMContentLoaded",
          details: { frameId: 0,
                     processId: 0,
                     tabId: 0,
                     timeStamp: 0,
                     url: getServerURL(path, unprotectedDomain) }},
        { label: "a-onCompleted",
          event: "onCompleted",
          details: { frameId: 0,
                     processId: 0,
                     tabId: 0,
                     timeStamp: 0,
                     url: getServerURL(path, unprotectedDomain) }},
        { label: "b-onBeforeNavigate",
          event: "onBeforeNavigate",
          details: { frameId: 0,
                     parentFrameId: -1,
                     processId: -1,
                     tabId: 0,
                     timeStamp: 0,
                     url: getServerURL(path, unprotectedDomain) }},
        { label: "b-onCommitted",
          event: "onCommitted",
          details: { frameId: 0,
                     processId: 0,
                     tabId: 0,
                     timeStamp: 0,
                     transitionQualifiers: [],
                     transitionType: "link",
                     url: getServerURL(path, unprotectedDomain) }},
        { label: "b-onDOMContentLoaded",
          event: "onDOMContentLoaded",
          details: { frameId: 0,
                     processId: 0,
                     tabId: 0,
                     timeStamp: 0,
                     url: getServerURL(path, unprotectedDomain) }},
        { label: "b-onCompleted",
          event: "onCompleted",
          details: { frameId: 0,
                     processId: 0,
                     tabId: 0,
                     timeStamp: 0,
                     url: getServerURL(path, unprotectedDomain) }}],
        [ navigationOrder("a-"), navigationOrder("b-") ]);
      navigateAndWait(getServerURL(path, protectedDomain),
          function() {
            navigateAndWait(getServerURL(path, unprotectedDomain),
                chrome.test.callbackPass())
          })
    }
  ]);
};
