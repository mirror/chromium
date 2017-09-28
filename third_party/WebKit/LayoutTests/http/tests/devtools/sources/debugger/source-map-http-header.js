// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that SourceMap and X-SourceMap http headers are propagated to scripts in the front-end.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.showPanel('sources');
  await TestRunner.addScriptTag('resources/source-map.php');
  await TestRunner.addScriptTag('resources/x-source-map.php');

  SourcesTestRunner.setQuiet(true);
  SourcesTestRunner.startDebuggerTest(step1);

  function step1() {
    TestRunner.addResult('Reloading...');

    var debuggerModel = TestRunner.debuggerModel;
    debuggerModel.addEventListener(SDK.DebuggerModel.Events.ParsedScriptSource, onScriptAdded);
    function onScriptAdded(event) {
      var script = event.data;
      if (!event.data.contentURL().endsWith('.php'))
        return;
      TestRunner.addResult('Added script:');
      TestRunner.addResult('  url: ' + script.sourceURL);
      TestRunner.addResult('  sourceMapUrl: ' + script.sourceMapURL);
    }

    TestRunner.reloadPage(step2);
  }

  function step2() {
    TestRunner.addResult('Finished reload.');
    SourcesTestRunner.completeDebuggerTest();
  }
})();
