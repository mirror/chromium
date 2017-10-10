// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that Elements panel allows to change src attribute on iframes inside inspected page. See bug 41350.\n`);
  await TestRunner.loadModule('elements_test_runner');
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.loadHTML(`
      <iframe src="resources/iframe-from-different-domain-data.html" id="receiver" onload="onIFrameLoad()"></iframe>
          <p>
          Tests that Elements panel allows to change src attribute on iframes
          inside inspected page.
          See <a href="https://bugs.webkit.org/show_bug.cgi?id=41350">bug 41350</a>.
          </p>
    `);
  await TestRunner.evaluateInPagePromise(`
      var onIFrameLoadCalled = false;
      function onIFrameLoad()
      {
          if (onIFrameLoadCalled)
              return;
          onIFrameLoadCalled = true;    }
  `);

  ElementsTestRunner.nodeWithId('receiver', step1);

  function step1(node) {
    node.setAttribute('src', 'src="http://localhost:8000/devtools/resources/iframe-from-different-domain-data.html"');
    ConsoleTestRunner.addConsoleSniffer(step2);
  }

  function step2() {
    ConsoleTestRunner.dumpConsoleMessages();
    TestRunner.completeTest();
  }
})();
