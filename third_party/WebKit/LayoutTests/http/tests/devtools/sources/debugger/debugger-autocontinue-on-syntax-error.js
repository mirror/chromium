// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that debugger won't stop on syntax errors even if "pause on uncaught exceptions" is on.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.loadHTML(`
      <p>
      Tests that debugger won't stop on syntax errors even if &quot;pause on uncaught exceptions&quot; is on.
      </p>
    `);
  await TestRunner.evaluateInPagePromise(`
      function loadIframe()
      {
          var iframe = document.createElement("iframe");
          iframe.src = relativeToTest("resources/syntax-error.html");
          document.body.appendChild(iframe);
      }
  `);

  SourcesTestRunner.startDebuggerTest(step1);

  function step1() {
    TestRunner.DebuggerAgent.setPauseOnExceptions(SDK.DebuggerModel.PauseOnExceptionsState.PauseOnUncaughtExceptions);
    ConsoleTestRunner.addConsoleSniffer(step2);
    TestRunner.evaluateInPage('loadIframe()');
  }

  function step2() {
    SourcesTestRunner.completeDebuggerTest();
  }
})();
