// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult('Tests setting breakpoint.');
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  await TestRunner.addScriptTag('resources/a.js');

  TestRunner.runTestSuite([
    async function setBreakpoint(next) {
      const sourceFrame = await SourcesTestRunner.showScriptSourcePromise('a.js');
      TestRunner.addResult('Set breakpoint');
      SourcesTestRunner.toggleBreakpoint(sourceFrame, 9, false);

      TestRunner.addResult('Dump breakpoints');
      await SourcesTestRunner.waitJavaScriptSourceFrameBreakpoints(sourceFrame);
      SourcesTestRunner.dumpJavaScriptSourceFrameBreakpoints(sourceFrame);
      dumpBreakpointStorage();

      TestRunner.addResult('Disable breakpoint');
      SourcesTestRunner.toggleBreakpoint(sourceFrame, 9, true);
      TestRunner.addResult('Dump breakpoints');
      await SourcesTestRunner.waitJavaScriptSourceFrameBreakpoints(sourceFrame);
      SourcesTestRunner.dumpJavaScriptSourceFrameBreakpoints(sourceFrame);
      dumpBreakpointStorage();

      TestRunner.addResult('Delete breakpoint');
      SourcesTestRunner.toggleBreakpoint(sourceFrame, 9, false);
      TestRunner.addResult('Dump breakpoints');
      await SourcesTestRunner.waitJavaScriptSourceFrameBreakpoints(sourceFrame);
      SourcesTestRunner.dumpJavaScriptSourceFrameBreakpoints(sourceFrame);
      dumpBreakpointStorage();

      next();
    },

    async function setDisabledBreakpoint(next) {
      const sourceFrame = await SourcesTestRunner.showScriptSourcePromise('a.js');

      TestRunner.addResult('Set disabled breakpoint');
      SourcesTestRunner.createNewBreakpoint(sourceFrame, 9, '', false);

      TestRunner.addResult('Dump breakpoints');
      await SourcesTestRunner.waitJavaScriptSourceFrameBreakpoints(sourceFrame);
      SourcesTestRunner.dumpJavaScriptSourceFrameBreakpoints(sourceFrame);
      dumpBreakpointStorage();

      TestRunner.addResult('Enable breakpoint');
      SourcesTestRunner.toggleBreakpoint(sourceFrame, 9, true);
      TestRunner.addResult('Dump breakpoints');
      await SourcesTestRunner.waitJavaScriptSourceFrameBreakpoints(sourceFrame);
      SourcesTestRunner.dumpJavaScriptSourceFrameBreakpoints(sourceFrame);
      dumpBreakpointStorage();

      TestRunner.addResult('Delete breakpoint');
      SourcesTestRunner.toggleBreakpoint(sourceFrame, 9, false);
      TestRunner.addResult('Dump breakpoints');
      await SourcesTestRunner.waitJavaScriptSourceFrameBreakpoints(sourceFrame);
      SourcesTestRunner.dumpJavaScriptSourceFrameBreakpoints(sourceFrame);
      dumpBreakpointStorage();

      next();
    },

    async function setConditionalBreakpoint(next) {
      const sourceFrame = await SourcesTestRunner.showScriptSourcePromise('a.js');

      TestRunner.addResult('Set conditional breakpoint');
      SourcesTestRunner.createNewBreakpoint(sourceFrame, 9, 'condition', true);

      TestRunner.addResult('Dump breakpoints');
      await SourcesTestRunner.waitJavaScriptSourceFrameBreakpoints(sourceFrame);
      SourcesTestRunner.dumpJavaScriptSourceFrameBreakpoints(sourceFrame);
      dumpBreakpointStorage();

      const lineDecorations = sourceFrame._lineBreakpointDecorations(9);
      lineDecorations[0].breakpoint.setCondition('');
      await SourcesTestRunner.waitJavaScriptSourceFrameBreakpoints(sourceFrame);
      SourcesTestRunner.dumpJavaScriptSourceFrameBreakpoints(sourceFrame);
      dumpBreakpointStorage();

      next();
    }
  ]);

  function dumpBreakpointStorage() {
    var breakpoints = Bindings.breakpointManager._storage._setting.get();
    TestRunner.addResult('Dumping breakpoint storage');
    for (const breakpoint of breakpoints)
      TestRunner.addResult(`  ${pathToFileName(breakpoint.url)}:${breakpoint.lineNumber} enabled: ${breakpoint.enabled} condition: ${breakpoint.condition}`);

    function pathToFileName(path) {
      return path.substring(path.lastIndexOf('/') + 1);
    }
  }
})();
