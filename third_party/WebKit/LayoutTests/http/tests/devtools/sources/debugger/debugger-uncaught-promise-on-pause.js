// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests uncaught promise rejections fired during pause.\n`);
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.evaluateInPagePromise(`
      function testFunction()
      {
          console.clear();
          debugger;
      }

      function runPromises(source)
      {
          Promise.reject(new Error(source + ".err1"))
              .then()
              .then()
              .then(); // Last is unhandled.

          var reject
          var m0 = new Promise(function(res, rej) { reject = rej; });
          var m1 = m0.then(function() {});
          var m2 = m0.then(function() {});
          var m3 = m0.then(function() {});
          var m4 = 0;
          m0.catch(function() {
              m2.catch(function() {
                  m1.catch(function() {
                      m4 = m3.then(function() {}); // Unhandled.
                  });
              });
          });
          reject(new Error(source + ".err2"));
      }

      function runPromisesFromInspector()
      {
          // setTimeout to cut off VM call frames from the stack trace.
          setTimeout(function timeout() {
              runPromises("inspector")
          }, 0);
      }
  `);

  SourcesTestRunner.setQuiet(true);
  SourcesTestRunner.startDebuggerTest(step1);

  function step1() {
    ConsoleTestRunner.addConsoleViewSniffer(addMessage, true);
    SourcesTestRunner.runTestFunctionAndWaitUntilPaused(didPause);
  }

  function didPause(callFrames, reason, breakpointIds, asyncStackTrace) {
    TestRunner.evaluateInPage('runPromisesFromInspector()', resumeExecution);
  }

  function resumeExecution() {
    SourcesTestRunner.resumeExecution();
  }

  var count = 0;
  function addMessage(uiMessage) {
    if (uiMessage.toString().indexOf('inspector.err') !== -1)
      ++count;
    if (count === 2)
      ConsoleTestRunner.expandConsoleMessages(dump);
  }

  function dump() {
    ConsoleTestRunner.dumpConsoleMessagesIgnoreErrorStackFrames();
    TestRunner.completeTest();
  }
})();
