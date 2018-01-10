// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Focus needs to be removed from the 'Cancel' button, before the next test is
// run. Otherwise, events associated with that button getting focused are not
// triggered (since it is already in focus when the test calls focus() on it).
function blurCancelButtonAfterTest() {
  // Since calling blur() on the 'Cancel' button results in an "blur is not a
  // function" error, requesting focus on the 'Ok' button, to remove focus from
  // the 'Cancel' button.
  rootNode.firstChild.children[0].focus();
}

var allTests = [
  function testEventListenerTarget() {
    var cancelButton = rootNode.firstChild.children[2];
    assertEq('Cancel', cancelButton.name);
    cancelButton.addEventListener(EventType.FOCUS,
                                  function onFocusTarget(event) {
      window.setTimeout(function() {
        cancelButton.removeEventListener(EventType.FOCUS, onFocusTarget);
        blurCancelButtonAfterTest();
        chrome.test.succeed();
      }, 0);
    });
    cancelButton.focus();
  },
  function testEventListenerBubble() {
    var cancelButton = rootNode.firstChild.children[2];
    assertEq('Cancel', cancelButton.name);
    var cancelButtonGotEvent = false;
    cancelButton.addEventListener(EventType.FOCUS,
                                  function onFocusBubble(event) {
      cancelButtonGotEvent = true;
      cancelButton.removeEventListener(EventType.FOCUS, onFocusBubble);
    });
    rootNode.addEventListener(EventType.FOCUS,
                               function onFocusBubbleRoot(event) {
      assertEq('focus', event.type);
      assertEq(cancelButton, event.target);
      assertTrue(cancelButtonGotEvent);
      rootNode.removeEventListener(EventType.FOCUS, onFocusBubbleRoot);
      blurCancelButtonAfterTest();
      chrome.test.succeed();
    });
    cancelButton.focus();
  },
  function testStopPropagation() {
    var cancelButton = rootNode.firstChild.children[2];
    assertEq('Cancel', cancelButton.name);
    function onFocusStopPropRoot(event) {
      rootNode.removeEventListener(EventType.FOCUS, onFocusStopPropRoot);
      chrome.test.fail("Focus event was propagated to root");
    };
    cancelButton.addEventListener(EventType.FOCUS,
                                  function onFocusStopProp(event) {
      cancelButton.removeEventListener(EventType.FOCUS, onFocusStopProp);
      event.stopPropagation();
      window.setTimeout((function() {
        rootNode.removeEventListener(EventType.FOCUS, onFocusStopPropRoot);
        blurCancelButtonAfterTest();
        chrome.test.succeed();
      }).bind(this), 0);
    });
    rootNode.addEventListener(EventType.FOCUS, onFocusStopPropRoot);
    cancelButton.focus();
  },
  function testEventListenerCapture() {
    var cancelButton = rootNode.firstChild.children[2];
    assertEq('Cancel', cancelButton.name);
    var cancelButtonGotEvent = false;
    function onFocusCapture(event) {
      cancelButtonGotEvent = true;
      cancelButton.removeEventListener(EventType.FOCUS, onFocusCapture);
      chrome.test.fail("Focus event was not captured by root");
    };
    cancelButton.addEventListener(EventType.FOCUS, onFocusCapture);
    rootNode.addEventListener(EventType.FOCUS,
                               function onFocusCaptureRoot(event) {
      assertEq('focus', event.type);
      assertEq(cancelButton, event.target);
      assertFalse(cancelButtonGotEvent);
      event.stopPropagation();
      rootNode.removeEventListener(EventType.FOCUS, onFocusCaptureRoot);
      rootNode.removeEventListener(EventType.FOCUS, onFocusCapture);
      window.setTimeout((function () {
        blurCancelButtonAfterTest();
        chrome.test.succeed();
      }).bind(this), 0);
    }, true);
    cancelButton.focus();
  }
];

setUpAndRunTests(allTests)
