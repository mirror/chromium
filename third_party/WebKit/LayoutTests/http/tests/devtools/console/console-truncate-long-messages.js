// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult('Tests that console logging large messages will be truncated.\n');

  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('console');

  var consoleView = Console.ConsoleView.instance();

  var longConsoleCommand = `"b".repeat(10000 * 20)`;
  var longPageCommand = `console.log("a".repeat(10000 * 2))`;
  TestRunner.addResult('Evaluating in console: ' + longConsoleCommand);
  TestRunner.addResult('Evaluating in page: ' + longPageCommand);

  await ConsoleTestRunner.evaluateInConsolePromise(longConsoleCommand);
  await TestRunner.evaluateInPagePromise(longPageCommand);

  dumpMessageLengths();

  TestRunner.addResult('\nExpanding hidden texts');
  consoleView._visibleViewMessages.forEach(message => {
    message.element().querySelectorAll('.console-expand-button').forEach(button => button.click());
  });

  dumpMessageLengths();
  TestRunner.completeTest();

  function dumpMessageLengths() {
    consoleView._visibleViewMessages.forEach((message, index) => {
      TestRunner.addResult("Message: " + index + ", length: " + consoleMessageText(index).length);
    });

    function consoleMessageText(index) {
      var messageElement = consoleView._visibleViewMessages[index].element();
      var anchor = messageElement.querySelector('.console-message-anchor');
      if (anchor)
        anchor.remove();
      return messageElement.deepTextContent();
    }
  }
})();
