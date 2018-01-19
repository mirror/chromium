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
  var wasShown = [];
  var willHide = [];

  // Measure visible/active ranges when in the middle.
  await logMessages(200);
  viewport.forceScrollItemToBeFirst(50);
  var {first, last} = ConsoleTestRunner.visibleIndices();
  var visibleCount = last - first + 1;
  var maxActiveCount = viewport._lastActiveIndex - viewport._firstActiveIndex + 1;
  // Use this because # of active messages below visible area may be different
  // # of active messages above visible area.
  var minActiveCount = 2 * (first - viewport._firstActiveIndex - 1) + visibleCount;
  var activeCountAbove = first - viewport._firstActiveIndex;
  var activeCountBelow = viewport._lastActiveIndex - last;
  TestRunner.addResult(`activeCountAbove: ${activeCountAbove}, activeCountBelow: ${activeCountBelow}`);
  clearConsoleAndReset();

  TestRunner.addResult('visibleCount: ' + visibleCount + ', maxActiveCount: ' + maxActiveCount);

  function onMessageShown() {
    wasShown.push(this);
  }

  function onMessageHidden() {
    willHide.push(this);
  }

  function printAndResetCounts() {
    TestRunner.addResult('Messages shown: ' + wasShown.length + ', hidden: ' + willHide.length);
    resetShowHideCounts();
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

  function clearConsoleAndReset() {
    Console.ConsoleView.clearConsole();
    resetShowHideCounts();
  }

  TestRunner.addSniffer(Console.ConsoleViewMessage.prototype, 'wasShown', onMessageShown, true);
  TestRunner.addSniffer(Console.ConsoleViewMessage.prototype, 'willHide', onMessageHidden, true);

  TestRunner.runTestSuite([
    async function addSmallCount(next) {
      clearConsoleAndReset();
      await logMessages(smallCount, false);
      printAndResetCounts();
      next();
    },

    async function addVisibleCount(next) {
      clearConsoleAndReset();
      await logMessages(visibleCount, false);
      printAndResetCounts();
      next();
    },

    async function addMaxActiveCount(next) {
      clearConsoleAndReset();
      await logMessages(maxActiveCount, false);
      printAndResetCounts();
      next();
    },

    async function addMoreThanMaxActiveCount(next) {
      clearConsoleAndReset();
      await logMessages(maxActiveCount, false);
      await logMessages(smallCount, false);
      printAndResetCounts();
      next();
    },

    async function scrollToBottomInPartialActiveWindow(next) {
      clearConsoleAndReset();
      var visiblePlusHalfExtraRows = visibleCount + Math.floor((minActiveCount - visibleCount) / 2) - 1;
      await logMessages(visiblePlusHalfExtraRows, false);
      viewport.forceScrollItemToBeFirst(0);
      resetShowHideCounts();
      viewport.element.scrollTop = 1000000;
      viewport.refresh();
      printAndResetCounts();
      next();
    },

    async function scrollToBottomInActiveWindow(next) {
      clearConsoleAndReset();
      await logMessages(minActiveCount, false);
      viewport.forceScrollItemToBeFirst(0);
      resetShowHideCounts();
      viewport.element.scrollTop = 1000000;
      viewport.refresh();
      printAndResetCounts();
      next();
    },

    async function scrollToBottomInMoreThanActiveWindow(next) {
      clearConsoleAndReset();
      await logMessages(maxActiveCount + smallCount, false);
      viewport.forceScrollItemToBeFirst(0);
      resetShowHideCounts();
      viewport.element.scrollTop = 1000000;
      viewport.refresh();
      printAndResetCounts();
      next();
    },

    async function logRepeatingMessages(next) {
      clearConsoleAndReset();
      await logMessages(visibleCount, true);
      printAndResetCounts();
      next();
    },

    async function reorderingMessages(next) {
      clearConsoleAndReset();
      await logMessages(smallCount, false);
      printAndResetCounts();
      resetShowHideCounts();
      TestRunner.addResult('Swapping messages 0 and 1');
      var temp = consoleView._visibleViewMessages[0];
      consoleView._visibleViewMessages[0] = consoleView._visibleViewMessages[1];
      consoleView._visibleViewMessages[1] = temp;
      viewport.invalidate();
      printAndResetCounts();
      next();
    }
  ]);
})();
