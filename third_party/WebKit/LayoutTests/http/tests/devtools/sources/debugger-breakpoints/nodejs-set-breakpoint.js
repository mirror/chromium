// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verify that front-end is able to set breakpoint for node.js scripts.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.loadModule('sdk_test_runner');
  await TestRunner.showPanel('sources');

  SDK.targetManager.mainTarget().setIsNodeJSForTest();
  SourcesTestRunner.startDebuggerTest();

  var debuggerModel = SDK.targetManager.mainTarget().model(SDK.DebuggerModel);
  var originalParsedScriptSource = TestRunner.override(debuggerModel, '_parsedScriptSource', overrideSourceURL, true);

  function overrideSourceURL(scriptId, sourceURL, startLine, startColumn, endLine, endColumn, executionContextId, hash,
               executionContextAuxData, isLiveEdit, sourceMapURL, hasSourceURL, hasSyntaxError, length) {
    // Overwrite hasSourceURL to pretend we're Node.js
    hasSourceURL = false;
    originalParsedScriptSource.call(this, scriptId, sourceURL, startLine, startColumn, endLine, endColumn, executionContextId, hash,
               executionContextAuxData, isLiveEdit, sourceMapURL, hasSourceURL, hasSyntaxError, length);
  }

  var functionText = 'function foobar() { \nconsole.log(\'foobar execute!\');\n}';
  var sourceURL = '\n//# sourceURL=/usr/local/home/prog/foobar.js';
  await TestRunner.evaluateInPageAnonymously(functionText + sourceURL);
  SourcesTestRunner.showScriptSource('foobar.js', didShowScriptSource);


  function didShowScriptSource(sourceFrame) {
    TestRunner.addResult('Setting breakpoint:');
    TestRunner.addSniffer(
        Bindings.BreakpointManager.ModelBreakpoint.prototype, '_addResolvedLocation', breakpointResolved);
    SourcesTestRunner.setBreakpoint(sourceFrame, 1, '', true);
  }

  function breakpointResolved(location) {
    SourcesTestRunner.waitUntilPaused(paused);
    TestRunner.evaluateInPage('foobar()');
  }

  function paused() {
    TestRunner.addResult('Successfully paused on breakpoint');
    SourcesTestRunner.completeDebuggerTest();
  }
})();
