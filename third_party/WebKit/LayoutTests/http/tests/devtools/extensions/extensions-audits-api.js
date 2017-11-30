// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests audits support in WebInspector Extensions API\n`);
  await TestRunner.loadModule('extensions_test_runner');
  await TestRunner.loadModule('audits_test_runner');
  await TestRunner.showPanel('audits');

  await ExtensionsTestRunner.runExtensionTests([
    function extension_testAuditsAPI(nextTest) {
      function onStartAuditCategory(results) {
        output("category.onAuditStarted fired, results dump follows:");
        dumpObject(results);
        var node = results.createResult("Subtree");
        dumpObject(node);
        // Make sure dumpObject() pushes stuff through before we continue.
        evaluateOnFrontend("TestRunner.deprecatedRunAfterPendingDispatches(reply)", function() {
            results.done();
        });
      }
      var category = webInspector.audits.addCategory("Extension audits");
      category.onAuditStarted.addListener(onStartAuditCategory);
      output("Added audit category, result dump follows:");
      dumpObject(category);
      extension_runAudits(nextTest);
    }
  ]);
})();
