// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests V8 cache information of Service Worker Cache Storage in timeline\n`);

  TestRunner.addResult('>>>> Loading performance_test_runner');
  await TestRunner.loadModule('performance_test_runner');

  TestRunner.addResult('>>>> Loading application_test_runner');
  await TestRunner.loadModule('application_test_runner');

  // Note: every test that uses a storage API must manually clean-up state from previous tests.
  TestRunner.addResult('>>>> Resetting state');
  await ApplicationTestRunner.resetState();

  TestRunner.addResult('>>>> Showing resources panel');
  await TestRunner.showPanel('resources');

  TestRunner.addResult('>>>> Showing timeline panel');
  await TestRunner.showPanel('timeline');

  TestRunner.addResult('>>>> evaluateInPagePromise');
  await TestRunner.evaluateInPagePromise(`
      function registerServiceWorkerAndwaitForActivated() {
        const script = 'resources/v8-cache-worker.js';
        const scope = 'resources/v8-cache-iframe.html';
        return registerServiceWorker(script, scope)
          .then(() => waitForActivated(scope));
      }
      function loadScript() {
        const url = '/devtools/resources/v8-cache-script.js';
        const frameId = 'frame_id';
        let iframeWindow = document.getElementById(frameId).contentWindow;
        return iframeWindow.loadScript(url)
          .then(() => iframeWindow.loadScript(url));
      }
  `);

  const scope = 'resources/v8-cache-iframe.html';
  const frameId = 'frame_id';

  TestRunner.addResult('>>>> invokeAsyncWithTimeline');
  await PerformanceTestRunner.invokeAsyncWithTimeline('registerServiceWorkerAndwaitForActivated');
  TestRunner.addResult('--- Trace events while installing -------------');
  PerformanceTestRunner.printTimelineRecordsWithDetails(
      TimelineModel.TimelineModel.RecordType.CompileScript);
  TestRunner.addResult('-----------------------------------------------');
  await ApplicationTestRunner.waitForActivated(scope);
  await TestRunner.addIframe(scope, {id: frameId});
  await PerformanceTestRunner.invokeAsyncWithTimeline('loadScript');
  TestRunner.addResult('--- Trace events while executing scripts ------');
  PerformanceTestRunner.printTimelineRecordsWithDetails(
      TimelineModel.TimelineModel.RecordType.CompileScript);
  TestRunner.addResult('-----------------------------------------------');
  TestRunner.completeTest();
})();
