// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests scrolling behavior in console prompt with multiline content.\n`);
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');
  ConsoleTestRunner.fixConsoleViewportDimensions(600, 200);
  await ConsoleTestRunner.waitUntilConsoleEditorLoaded();

  var consoleView = Console.ConsoleView.instance();
  var viewport = consoleView._viewport;
  var prompt = consoleView._prompt;
  prompt.setText('foo\n'.repeat(1000));

  function moveCaretToFirstLine() {
    prompt._editor.setSelection(TextUtils.TextRange.createFromLocation(0, Infinity));
  }

  function moveCaretToLastLine() {
    prompt.moveCaretToEndOfPrompt();
  }

  function dumpScrollTop() {
    var isZero = viewport.element.scrollTop === 0;
    TestRunner.addResult(`ScrollTop is${isZero ? '' : ' not'} zero`);
  }

  function type() {
    eventSender.keyDown('A');
  }

  TestRunner.runTestSuite([
    function testPreserveScrollWhenTyping(next) {
      viewport.element.scrollTop = 0;
      moveCaretToFirstLine();
      type();
      dumpScrollTop();
      next();
    },

    async function testScrollWhenTypingWithCursorOffscreen(next) {
      moveCaretToLastLine();
      viewport.element.scrollTop = 0;
      type();
      dumpScrollTop();
      next();
    },

    async function testPreserveScrollWhenTypingWithMessages(next) {
      await TestRunner.evaluateInPagePromise(`console.log('foo');`);
      await ConsoleTestRunner.waitForConsoleMessages(1, () => {
        // Ensure that the message's element is in DOM.
        viewport.invalidate();

        viewport.element.scrollTop = 0;
        moveCaretToFirstLine();
        type();
        dumpScrollTop();
        next();
      });
    },

    async function testScrollWhenTypingWithCursorOffscreenWithMessages(next) {
      moveCaretToLastLine();
      viewport.element.scrollTop = 0;
      type();
      dumpScrollTop();
      next();
    },
  ]);
})();
