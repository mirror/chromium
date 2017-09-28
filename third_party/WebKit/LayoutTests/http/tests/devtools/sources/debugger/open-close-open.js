// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that scripts panel displays resource content correctly after the open - close - open sequence. Bug 56747\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.loadHTML(`
      <p>
      Tests that scripts panel displays resource content correctly after the open - close - open sequence.
      <a href="https://bugs.webkit.org/show_bug.cgi?id=56747">Bug 56747</a>
      </p>
    `);

  TestRunner.evaluateInPage('frontendReopeningCount', function(result) {
    if (result._description === '0')
      TestRunner.evaluateInPage('reopenFrontend()');
    else {
      SourcesTestRunner.runDebuggerTestSuite([function testSourceFrameContent(next) {
        SourcesTestRunner.showScriptSource('open-close-open.html', didShowScriptSource);

        function didShowScriptSource(sourceFrame) {
          SourcesTestRunner.dumpSourceFrameContents(sourceFrame);
          next();
        }
      }]);
    }
  });
})();
