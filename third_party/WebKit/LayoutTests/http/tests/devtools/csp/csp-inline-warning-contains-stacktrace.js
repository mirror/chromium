// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `This test injects an inline script from JavaScript. The resulting console error should contain a stack trace.\n`);
  await TestRunner.loadModule('console_test_runner');

  await TestRunner.loadHTML(`
    <!DOCTYPE html>
    <meta http-equiv='Content-Security-Policy' content="script-src 'self'">
  `);

  ConsoleTestRunner.addConsoleSniffer(addMessage);
  await TestRunner.evaluateInPagePromise(`
    function thisTest() {
      var s = document.createElement('script');
      s.innerText = "console.log(1)";
      document.head.appendChild(s);
    }
    thisTest();
  `);

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
