// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that all inlined scripts from the same document are shown in the same source frame with html script tags. Bug 54544.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.loadHTML(`
      <p>
      Tests that all inlined scripts from the same document are shown in the same source frame with html script tags.
      <a id="hash" href="https://bugs.webkit.org/show_bug.cgi?id=54544">Bug 54544.</a>
      </p>
    `);
  await TestRunner.evaluateInPagePromise(`
      function f4()
      {
          return 0;
      }
      f4();
    `);

  SourcesTestRunner.startDebuggerTest(step0, true);

  function step0() {
    TestRunner.evaluateInPage('window.location="#hash"', step1);
  }

  function step1(loc) {
    TestRunner.addResult('window.location: ' + loc.description);
    SourcesTestRunner.showScriptSource('debug-inlined-scripts-fragment-id.html', step2);
  }

  function step2(sourceFrame) {
    TestRunner.addResult('Script source was shown.');
    SourcesTestRunner.setBreakpoint(sourceFrame, 5, '', true);
    SourcesTestRunner.waitUntilPaused(step3);
    TestRunner.reloadPage(SourcesTestRunner.completeDebuggerTest.bind(SourcesTestRunner));
  }

  function step3(callFrames) {
    TestRunner.addResult('Script execution paused.');
    SourcesTestRunner.captureStackTrace(callFrames);
    SourcesTestRunner.showScriptSource('debug-inlined-scripts-fragment-id.html', step4);
  }

  function step4(sourceFrame) {
    SourcesTestRunner.dumpSourceFrameContents(sourceFrame);
    SourcesTestRunner.resumeExecution(SourcesTestRunner.waitUntilPaused.bind(null, step5));
  }

  function step5(callFrames) {
    if (callFrames[0].location.lineNumber !== 9) {
      SourcesTestRunner.resumeExecution(SourcesTestRunner.waitUntilPaused.bind(null, step5));
      return;
    }
    TestRunner.addResult('Script execution paused.');
    SourcesTestRunner.captureStackTrace(callFrames);
    SourcesTestRunner.showScriptSource('debug-inlined-scripts-fragment-id.html', step6);
  }

  function step6(sourceFrame) {
    SourcesTestRunner.dumpSourceFrameContents(sourceFrame);
    SourcesTestRunner.resumeExecution(SourcesTestRunner.waitUntilPaused.bind(null, step7));
  }

  function step7() {
    SourcesTestRunner.resumeExecution(SourcesTestRunner.waitUntilPaused.bind(null, step5));
  }
})();
