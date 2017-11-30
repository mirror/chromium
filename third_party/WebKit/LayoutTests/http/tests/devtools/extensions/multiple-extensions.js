// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests co-existance of multiple DevTools extensions\n`);
  await TestRunner.loadModule('extensions_test_runner');

  const tests = [
    function extension_testCreatePanel(nextTest) {
      function onPanelCreated(panel) {
        output("Panel created");
        dumpObject(panel);
        nextTest();
      }
      var basePath = location.pathname.replace(/\/[^/]*$/, "/");
      webInspector.panels.create("Test Panel", basePath + "extension-panel.png", basePath + "extension-panel.html", onPanelCreated);
    },

    function extension_onTestsDone() {
      evaluateOnFrontend("TestRunner.extensionTestComplete();");
    }
  ];

  await TestRunner.evaluateInPagePromise(tests.join('\n'));

  var pendingTestCount = 2;
  TestRunner.extensionTestComplete = function() {
    if (--pendingTestCount)
      ExtensionsTestRunner.runExtensionTests();
    else
      TestRunner.completeTest();
  }

  Common.moduleSetting("shortcutPanelSwitch").set(true);
  await ExtensionsTestRunner.runExtensionTests();
})();
