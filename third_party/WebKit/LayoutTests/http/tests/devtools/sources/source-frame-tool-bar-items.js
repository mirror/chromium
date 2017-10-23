// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult('Checks tool bar items for anonymous scripts');
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.showPanel('sources');

  await SourcesTestRunner.startDebuggerTestPromise();
  TestRunner.evaluateInPagePromise(`
    function testFunction() {
      eval('debugger;');
    }
    //# sourceURL=foo.js`);

  await SourcesTestRunner.runTestFunctionAndWaitUntilPausedPromise();
  var panel = UI.panels.sources;
  var sourceFrame = panel.visibleView;
  var items = sourceFrame.syncToolbarItems();
  for (let item of items) {
    TestRunner.addResult(item.element.deepTextContent());
  }
  SourcesTestRunner.completeDebuggerTest();
})();
