// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function test() {
  TestRunner.addResult('Checks that we update breakpoint location if source map request is slow');
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  await SourcesTestRunner.startDebuggerTestPromise();

  TestRunner.evaluateInPageAnonymously(`function foo() {
  console.log(42);
}
//# sourceMappingURL=${TestRunner.url('../resources/a.js.map')}`);

  let sourceFrame = await new Promise(resolve => SourcesTestRunner.showScriptSource('a.ts', resolve));
  SourcesTestRunner.toggleBreakpoint(sourceFrame, 1, false);
  await SourcesTestRunner.waitBreakpointSidebarPane(true);
  SourcesTestRunner.dumpBreakpointSidebarPane();

  await new Promise(resolve => TestRunner.reloadPage(resolve));

  let origLoad = Host.ResourceLoader.load;
  let continueRequest;
  Host.ResourceLoader.load = function(){
    let url = arguments[0];
    if (url.endsWith('a.js.map')) {
      continueRequest = () => origLoad.apply(this, arguments);
      return;
    }
    return origLoad.apply(this, arguments);
  }

  await TestRunner.evaluateInPageAnonymously(`function foo() {
  console.log(42);
}
//# sourceMappingURL=${TestRunner.url('../resources/a.js.map')}`);

  await SourcesTestRunner.waitBreakpointSidebarPane(true);
  SourcesTestRunner.dumpBreakpointSidebarPane();
  continueRequest();
  await SourcesTestRunner.waitBreakpointSidebarPane(true);
  SourcesTestRunner.dumpBreakpointSidebarPane();
  SourcesTestRunner.completeDebuggerTest();
})();
