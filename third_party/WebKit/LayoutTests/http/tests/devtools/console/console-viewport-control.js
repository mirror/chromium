// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verifies viewport correctly shows and hides messages while logging and scrolling.\n`);
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');
  await TestRunner.evaluateInPagePromise(`
      function addMessages(count)
      {
          for (var i = 0; i < count; ++i)
              console.log("Message #" + i);
      }

      function addRepeatingMessages(count)
      {
          for (var i = 0; i < count; ++i)
              console.log("Repeating message");
      }

      //# sourceURL=console-viewport-control.js
    `);

  const viewportHeight = 200;
  ConsoleTestRunner.fixConsoleViewportDimensions(600, viewportHeight);
  var consoleView = Console.ConsoleView.instance();
  var viewport = consoleView._viewport;
  const smallCount = 3;
  const rowHeight = viewportHeight / consoleView.minimumRowHeight();
  const maxVisibleCount = Math.ceil(rowHeight);
  const maxActiveCount = Math.ceil(rowHeight * 2);
  var wasShown = [];
  var willHide = [];

  TestRunner.addResult('Max visible messages count: ' + maxVisibleCount + ', active count: ' + maxActiveCount);

  function onMessageShown() {
    wasShown.push(this);
  }

  function onMessageHidden() {
    willHide.push(this);
  }

  function printAndResetCounts(next) {
    TestRunner.addResult('Messages shown: ' + wasShown.length + ', hidden: ' + willHide.length);
    resetShowHideCounts();
    next();
  }

  function resetShowHideCounts() {
    wasShown = [];
    willHide = [];
  }

  function logMessages(count, repeating) {
    TestRunner.addResult('Logging ' + count + ' messages');
    return new Promise(resolve => {
      var awaitingMessagesCount = count;
      function messageAdded() {
        if (!--awaitingMessagesCount) {
          viewport.invalidate();
          resolve();
        } else {
          ConsoleTestRunner.addConsoleSniffer(messageAdded, false);
        }
      }
      ConsoleTestRunner.addConsoleSniffer(messageAdded, false);
      if (!repeating)
        TestRunner.evaluateInPage(String.sprintf('addMessages(%d)', count));
      else
        TestRunner.evaluateInPage(String.sprintf('addRepeatingMessages(%d)', count));
    });
  }

  function printStuckToBottom() {
    TestRunner.addResult('Is at bottom: ' + viewport.element.isScrolledToBottom());
  }

  function clearConsoleAndReset() {
    Console.ConsoleView.clearConsole();
    resetShowHideCounts();
  }

  // The viewport element's bounding box may include padding/margins.
  // Measure these and treat messages in the padding/margin as NOT visible.
  await logMessages(1, false);
  var viewportRect = viewport.element.getBoundingClientRect();
  var firstRect = consoleView._visibleViewMessages[0]._element.getBoundingClientRect();
  var viewportMargin = firstRect.top - viewportRect.top;
  TestRunner.addResult(`Viewport element has margin of ${viewportMargin}px`);
  clearConsoleAndReset();

  TestRunner.addSniffer(Console.ConsoleViewMessage.prototype, 'wasShown', onMessageShown, true);
  TestRunner.addSniffer(Console.ConsoleViewMessage.prototype, 'willHide', onMessageHidden, true);

  TestRunner.runTestSuite([
    async function testFirstLastVisibleIndices(next) {
      dumpVisibleIndices();
      await logMessages(100, false);
      viewport.forceScrollItemToBeFirst(0);
      dumpVisibleIndices();
      viewport.forceScrollItemToBeFirst(1);
      dumpVisibleIndices();
      viewport.element.scrollTop += (consoleView.minimumRowHeight() - 1);
      viewport.refresh();
      dumpVisibleIndices();
      viewport.forceScrollItemToBeLast(50);
      dumpVisibleIndices();
      viewport.forceScrollItemToBeLast(99);
      dumpVisibleIndices();
      next();

      function dumpVisibleIndices() {
        var first = -1;
        var last = -1;
        var viewportRect = viewport.element.getBoundingClientRect();
        for (var i = 0; i < consoleView._visibleViewMessages.length; i++) {
          var item = consoleView._visibleViewMessages[i];
          if (!item._element)
            continue;
          var itemRect = item._element.getBoundingClientRect();
          if (first === -1 && itemRect.bottom > viewportRect.top + viewportMargin + 1)
            first = i;
          if (last === -1 && itemRect.bottom >= viewportRect.bottom - viewportMargin - 1)
            last = i;
        }
        var total = first === -1 ? 0 : (last - first + 1);
        TestRunner.addResult(`First visible: ${first}, Last visible: ${last}, Total visible: ${total}
First active: ${viewport._firstActiveIndex}, Last active: ${viewport._lastActiveIndex}\n`);
      }
    },

    async function addSmallCount(next) {
      clearConsoleAndReset();
      await logMessages(smallCount, false);
      printAndResetCounts(next);
    },

    async function addMaxVisibleCount(next) {
      clearConsoleAndReset();
      await logMessages(maxVisibleCount, false);
      printAndResetCounts(next);
    },

    async function addMaxActiveCount(next) {
      clearConsoleAndReset();
      await logMessages(maxActiveCount, false);
      printAndResetCounts(next);
      printStuckToBottom();
    },

    async function addMoreThanMaxActiveCount(next) {
      clearConsoleAndReset();
      await logMessages(maxActiveCount, false);
      step2();
      async function step2() {
        await logMessages(smallCount, false);
        printAndResetCounts(next);
        printStuckToBottom();
      }
    },

    async function scrollUpWithinActiveWindow(next) {
      clearConsoleAndReset();
      await logMessages(maxActiveCount, false);
      step2();
      printStuckToBottom();
      async function step2() {
        resetShowHideCounts();
        viewport.forceScrollItemToBeFirst(0);
        printAndResetCounts(next);
      }
    },

    async function scrollUpToPositionOutsideOfActiveWindow(next) {
      clearConsoleAndReset();
      await logMessages(maxActiveCount + smallCount, false);
      step2();
      printStuckToBottom();
      async function step2() {
        resetShowHideCounts();
        viewport.forceScrollItemToBeFirst(0);
        printAndResetCounts(next);
      }
    },

    async function logRepeatingMessages(next) {
      clearConsoleAndReset();
      await logMessages(maxVisibleCount, true);
      printAndResetCounts(next);
    },

    async function reorderingMessages(next) {
      clearConsoleAndReset();
      await logMessages(smallCount, false);
      printAndResetCounts(step2);
      async function step2() {
        resetShowHideCounts();
        TestRunner.addResult('Swapping messages 0 and 1');
        var temp = consoleView._visibleViewMessages[0];
        consoleView._visibleViewMessages[0] = consoleView._visibleViewMessages[1];
        consoleView._visibleViewMessages[1] = temp;
        viewport.invalidate();
        printAndResetCounts(next);
      }
    }
  ]);
})();
