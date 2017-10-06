// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that valid parsed script notifications are received by front-end. Bug 52721\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.loadHTML(`
      <p>
      Tests that valid parsed script notifications are received by front-end.
      <a href="https://bugs.webkit.org/show_bug.cgi?id=52721">Bug 52721</a>
      </p>
    `);
  await TestRunner.evaluateInPagePromise(`
      function f1()
      {
      }
    `);
  await TestRunner.evaluateInPagePromise(`     function f2() {}
    `);
  await TestRunner.evaluateInPagePromise(`    function f3() {}
    `);
  await TestRunner.evaluateInPagePromise(`
         function f4() {}
    `);

  var scripts = [];
  SourcesTestRunner.startDebuggerTest(step1);

  function step1() {
    SourcesTestRunner.queryScripts(function(script) {
      step2({data: script});
    });
    TestRunner.debuggerModel.addEventListener(SDK.DebuggerModel.Events.ParsedScriptSource, step2);
  }

  function step2(event) {
    var script = event.data;
    if (!script.sourceURL.includes('debugger-scripts.js'))
      return;
    scripts.push(script);
    if (scripts.length === 4)
      step3();
  }

  function step3() {
    scripts.sort(function(x, y) {
      return x.lineOffset - y.lineOffset;
    });
    for (var i = 0; i < scripts.length; ++i) {
      TestRunner.addResult('script ' + (i + 1) + ':');
      TestRunner.addResult('    start: ' + scripts[i].lineOffset + ':' + scripts[i].columnOffset);
      TestRunner.addResult('    end: ' + scripts[i].endLine + ':' + scripts[i].endColumn);
    }
    SourcesTestRunner.completeDebuggerTest();
  }
})();
