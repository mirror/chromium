// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verifies viewport stick-to-bottom behavior.\n`);
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');
  await TestRunner.evaluateInPagePromise(`
      function populateConsoleWithMessages(count)
      {
          for (var i = 0; i < count - 1; ++i)
              console.log("Multiline\\nMessage #" + i);
          console.log("hello %cworld", "color: blue");
      }

      //# sourceURL=console-viewport-selection.js
    `);

  var viewportHeight = 200;
  ConsoleTestRunner.fixConsoleViewportDimensions(600, viewportHeight);
  var consoleView = Console.ConsoleView.instance();
  var viewport = consoleView._viewport;
  const minimumViewportMessagesCount = 10;
  const messagesCount = 150;
  const middleMessage = messagesCount / 2;
  var viewportMessagesCount;

  logMessagesToConsole(messagesCount, () => TestRunner.runTestSuite(testSuite));

  var testSuite = [
    function verifyViewportIsTallEnough(next) {
      viewport.invalidate();
      viewport.forceScrollItemToBeFirst(0);
      viewportMessagesCount = viewport.lastVisibleIndex() - viewport.firstVisibleIndex() + 1;
      if (viewportMessagesCount < minimumViewportMessagesCount) {
        TestRunner.addResult(String.sprintf(
            'Test cannot be run as viewport is not tall enough. It is required to contain at least %d messages, but %d only fit',
            minimumViewportMessagesCount, viewportMessagesCount));
        TestRunner.completeTest();
        return;
      }
      TestRunner.addResult(String.sprintf('Viewport contains %d messages', viewportMessagesCount));
      next();
    },

    function testScrollViewportToBottom(next) {
      consoleView._immediatelyScrollToBottom();
      dumpAndContinue(next);
    },

    function testConsoleSticksToBottom(next) {
      logMessagesToConsole(messagesCount, onMessagesDumped);

      function onMessagesDumped() {
        dumpAndContinue(next);
      }
    },

    function testSmoothScrollDoesNotStickToBottom(next) {
      TestRunner.addSniffer(Console.ConsoleView.prototype, '_updateViewportStickinessForTest', onUpdateTimeout);
      sendPageUp();

      function onUpdateTimeout() {
        dumpAndContinue(next);
      }
    }
  ];

  function sendPageUp() {
    var keyEvent = TestRunner.createKeyEvent('PageUp');
    consoleView._prompt.element.dispatchEvent(keyEvent);
    TestRunner.addResult('test setting scrollTop')
    viewport.element.scrollTop -= 10;
    TestRunner.addResult('scrollTop? ' + viewport.element.scrollTop)
    TestRunner.addResult('test is at bottom? ' + viewport.element.isScrolledToBottom())
    TestRunner.addResult('test done setting scrollTop')
  }

  function dumpAndContinue(callback) {
    viewport.refresh();
    TestRunner.addResult(
        'Is at bottom: ' + viewport.element.isScrolledToBottom() + ', should stick: ' + viewport.stickToBottom());
    callback();
  }

  function logMessagesToConsole(count, callback) {
    var awaitingMessagesCount = count;
    function messageAdded() {
      if (!--awaitingMessagesCount)
        callback();
      else
        ConsoleTestRunner.addConsoleSniffer(messageAdded, false);
    }

    ConsoleTestRunner.addConsoleSniffer(messageAdded, false);
    TestRunner.evaluateInPage(String.sprintf('populateConsoleWithMessages(%d)', count));
  }
})();
