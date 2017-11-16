// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that resources have proper documentURL set in the tree model.\n`);
  await TestRunner.loadModule('application_test_runner');
  await TestRunner.showPanel('resources');

  TestRunner.resourceTreeModel.addEventListener(SDK.ResourceTreeModel.Events.FrameNavigated, waitForResources);
  TestRunner.evaluateInPagePromise(`
    (function loadIframe() {
      var iframe = document.createElement("iframe");
      iframe.src = "${TestRunner.url('resources/dummy-iframe.html')}";
      document.body.appendChild(iframe);
    })();
  `);

  async function waitForResources() {
    TestRunner.resourceTreeModel.removeEventListener(SDK.ResourceTreeModel.Events.FrameNavigated, waitForResources);
    await TestRunner.waitForUISourceCode('dummy-iframe.html');
    ApplicationTestRunner.dumpResources(resource => resource.url + ' => ' + resource.documentURL);
    TestRunner.completeTest();
  }
})();
