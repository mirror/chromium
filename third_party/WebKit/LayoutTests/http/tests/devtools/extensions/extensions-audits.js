// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests audits support in WebInspector Extensions API\n`);
  await TestRunner.loadModule('extensions_test_runner');
  await TestRunner.loadModule('audits_test_runner');
  await TestRunner.showPanel('elements');
  await TestRunner.showPanel('audits');
  await TestRunner.loadHTML(`<span id="test-element"><b></b></span>`);
  await ExtensionsTestRunner.runExtensionTests([
    function extension_testAudits(nextTest) {
      var pendingOutput = [];

      function onStartAuditCategory(results) {
        pendingOutput.push("category.onAuditStarted fired");
        results.addResult("Passed rule", "this rule always passes ok", results.Severity.Info);
        results.addResult("Failed rule (42)", "this rule always fails", results.Severity.Severe);

        var node = results.createResult("Subtree");
        node.addChild("Some url: ", results.createURL("http://www.webkit.org", "WebKit"), " more text ", results.createURL("http://www.google.com"));
        var nestedNode = node.addChild("... and a snippet");
        nestedNode.expanded = true;
        nestedNode.addChild(results.createSnippet("function rand()\n{\n    return 4;\n}"));
        nestedNode.addChild(results.createResourceLink("file:///path/to/error.html", 10));
        nestedNode.addChild(results.createObject("({a: 42, b: 'foo'})", "object details"));
        nestedNode.addChild(results.createNode("document.getElementById('test-element')"));

        webInspector.inspectedWindow.eval("location.href", function(inspectedPageURL) {
          nestedNode.addChild(results.createResourceLink(inspectedPageURL, 20));

          evaluateOnFrontend("ExtensionsTestRunner.dumpAuditProgress(); reply();", function() {
            results.addResult("Rule with details subtree (1)", "This rule has a lot of details", results.Severity.Warning, node);
            results.updateProgress(10, 20);
            evaluateOnFrontend("ExtensionsTestRunner.dumpAuditProgress(); reply();", results.done.bind(results));
          });
        });
      }
      function onStartAuditFailedCategory() {
        pendingOutput.push("failedCategory.onAuditStarted fired, throwing exception");
        throw "oops!";
      }
      function onStartAuditDisabledCategory(results) {
        pendingOutput.push("FAIL: disabledCategory.onAuditStarted fired");
        results.done();
      }
      var category = webInspector.audits.addCategory("Extension audits");
      category.onAuditStarted.addListener(onStartAuditCategory);

      var failedCategory = webInspector.audits.addCategory("Extension audits that fail", 2);
      failedCategory.onAuditStarted.addListener(onStartAuditFailedCategory);

      var disabledCategory = webInspector.audits.addCategory("Disabled extension audits", 2);
      disabledCategory.onAuditStarted.addListener(onStartAuditDisabledCategory);

      function auditsDone() {
        pendingOutput.sort().forEach(output);
        nextTest();
      }
      extension_runAudits(auditsDone);
    }
  ]);
})();
