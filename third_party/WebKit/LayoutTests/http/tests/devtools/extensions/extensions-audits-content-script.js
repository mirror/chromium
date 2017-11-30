// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests audit formatters performing evals on content scripts in WebInspector Extensions API\n`);
  await TestRunner.loadModule('extensions_test_runner');
  await TestRunner.loadModule('audits_test_runner');
  await TestRunner.loadHTML(`<span id="test-element"><b></b></span>`);
  await TestRunner.showPanel('elements');
  await TestRunner.showPanel('audits');
  await TestRunner.evaluateInPagePromise(`
    window.whereAmI = "main world";

    testRunner.setIsolatedWorldSecurityOrigin(632, "http://devtools-extensions.oopif.test:8000");
    testRunner.evaluateScriptInIsolatedWorld(632, "window.whereAmI = 'brave new world'");
  `);

  await ExtensionsTestRunner.runExtensionTests([
    function extension_testAudits(nextTest) {
      var pendingOutput = [];

      function onStartAuditCategory(results) {
        pendingOutput.push("category.onAuditStarted fired");
        var node = results.createResult("Test Formatters");
        node.addChild(results.createObject("({whereAmI: window.whereAmI})", "main world object"));
        node.addChild(results.createNode("document.getElementById('test-element')"));

        node.addChild(results.createObject("({whereAmI: window.whereAmI})", "content script object", { useContentScriptContext: true }));
        node.addChild(results.createNode("document.getElementById('test-element')", { useContentScriptContext: true }));

        results.addResult("Rule with details subtree (1)", "", results.Severity.Warning, node);
        results.done();
      }
      var category = webInspector.audits.addCategory("Extension audits", 20);
      category.onAuditStarted.addListener(onStartAuditCategory);

      function auditsDone() {
        pendingOutput.sort().forEach(output);
        nextTest();
      }
      webInspector.inspectedWindow.eval("undefined", function() {
        extension_runAudits(auditsDone);
      });
    }
  ]);
})();
