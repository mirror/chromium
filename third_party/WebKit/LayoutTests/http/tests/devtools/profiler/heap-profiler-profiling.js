// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult('Tests that sampling heap profiling works.\n');
  await TestRunner.loadModule('profiler_test_runner');
  await TestRunner.showPanel('heap_profiler');

  ProfilerTestRunner.runProfilerTestSuite([async function testProfiling(next) {

    ProfilerTestRunner.showProfileWhenAdded('Profile 1');
    ProfilerTestRunner.waitUntilProfileViewIsShown('Profile 1', findPageFunctionInProfileView);
    ProfilerTestRunner.startSamplingHeapProfiler();
    await TestRunner.evaluateInPagePromise(`
        function pageFunction() {
          (function () {
            window.holder = [];
            // Allocate few MBs of data.
            for (var i = 0; i < 1000; ++i)
              window.holder.push(new Array(1000).fill(42));
          })();
        }
        pageFunction();`);
    ProfilerTestRunner.stopSamplingHeapProfiler();

    function checkFunction(tree, name, url) {
      let node = tree.children[0];
      if (!node)
        TestRunner.addResult('no node');
      while (node) {
        const nodeURL = node.element().children[2].lastChild.textContent;
        if (node.functionName === name && url === nodeURL) {
          TestRunner.addResult('found ' + name + ' ' + (url || ''));
          return;
        }
        node = node.traverseNextNode(true, null, true);
      }
      TestRunner.addResult(name + ' not found');
    }

    function findPageFunctionInProfileView(view) {
      const tree = view.profileDataGridTree;
      if (!tree)
        TestRunner.addResult('no tree');
      checkFunction(tree, 'pageFunction', 'heap-profiler-profiling.js:16');
      checkFunction(tree, '(anonymous)', 'heap-profiler-profiling.js:1');
      next();
    }
  }], UI.panels.heap_profiler);

})();
