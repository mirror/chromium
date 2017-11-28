// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
    TestRunner.addResult(
      `This test should trigger a CSP violation by attempting to evaluate a string with setInterval.\n`);
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.loadModule('sources_test_runner');

  await TestRunner.loadHTML(`
    <!DOCTYPE html>
    <meta http-equiv='Content-Security-Policy' content="script-src 'self' 'unsafe-inline'">
  `);

  await TestRunner.evaluateInPagePromise(`
      function thisTest() {
          var id = setInterval('log("FAIL")', 0);
          if (id != 0)
              log('FAIL: Return value for string (should be 0): ' + id);
          else
              log('PASS: Return value for blocked setInterval is 0');
      }
  `);
  ConsoleTestRunner.addConsoleSniffer(addMessage);
  await TestRunner.evaluateInPagePromise(`thisTest()`);

  function addMessage(message) {
    var viewMessages = Console.ConsoleView.instance()._visibleViewMessages;
    for (var i = 0; i < viewMessages.length; ++i) {
      var m = viewMessages[i].consoleMessage();
      TestRunner.addResult("Message[" + i + "]: " + Bindings.displayNameForURL(m.url) + ":" + m.line + " " + m.messageText);
      var trace = m.stackTrace ? m.stackTrace.callFrames : null;
      if (!trace) {
        TestRunner.addResult("FAIL: no stack trace attached to message #" + i);
      } else {
        TestRunner.addResult("Stack Trace:\n");
        TestRunner.addResult("  url: " + trace[0].url);
        TestRunner.addResult("  function: " + trace[0].functionName);
        TestRunner.addResult("  line: " + trace[0].lineNumber);
      }
    }
    TestRunner.completeTest();
  }

})();
