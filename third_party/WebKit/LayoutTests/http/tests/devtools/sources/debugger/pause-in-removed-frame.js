// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Parent Frame\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.loadHTML(`
      <iframe id="child" src="./resources/child.html" onload="runTest()"></iframe>
      <p>
      Tests &quot;pause&quot; functionality in detached frame.
      </p>
    `);
  await TestRunner.evaluateInPagePromise(`
      window.removeIframe = function()
      {
        var child = document.getElementById('child');
        child.parentNode.removeChild(child);
        debugger;
      };

      function testFunction()
      {
          var childWindow = document.getElementById("child").contentWindow;
          childWindow.sendRequest();
      }
  `);

  SourcesTestRunner.startDebuggerTest(step1);

  function step1() {
    SourcesTestRunner.runTestFunctionAndWaitUntilPaused(step2);
  }

  function step2(callFrames) {
    SourcesTestRunner.captureStackTrace(callFrames);
    SourcesTestRunner.completeDebuggerTest();
  }
})();
