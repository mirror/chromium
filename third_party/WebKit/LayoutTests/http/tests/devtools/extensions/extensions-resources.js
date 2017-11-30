// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests resource-related methods of WebInspector extension API\n`);
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.loadModule('extensions_test_runner');
  await TestRunner.loadModule('sources_test_runner');

  TestRunner.clickOnURL = function() {
    UI.viewManager.showView("console").then(() => {
      Console.ConsoleView.instance()._updateMessageList();
      var xpathResult = document.evaluate("//span[@class='devtools-link' and starts-with(., 'test-script.js')]",
                                          Console.ConsoleView.instance().element, null, XPathResult.ANY_UNORDERED_NODE_TYPE, null);

      var click = document.createEvent("MouseEvent");
      click.initMouseEvent("click", true, true);
      xpathResult.singleNodeValue.dispatchEvent(click);
    });
  }

  TestRunner.waitForStyleSheetChangedEvent = function(reply) {
    var originalSetTimeout = Common.Throttler.prototype._setTimeout;
    Common.Throttler.prototype._setTimeout = innerSetTimeout;
    TestRunner.addSniffer(SDK.CSSModel.prototype, "_fireStyleSheetChanged", onStyleSheetChanged);

    function onStyleSheetChanged() {
      Common.Throttler.prototype._setTimeout = originalSetTimeout;
      reply();
    }

    function innerSetTimeout(operation, timeout) {
      return originalSetTimeout.call(this, operation, 0);
    }
  }

  await TestRunner.evaluateInPagePromise(`
    function loadFrame() {
      var callback;
      var promise = new Promise((fulfill) => callback = fulfill);
      var iframe = document.createElement("iframe");
      iframe.src = "resources/subframe.html";
      iframe.addEventListener("load", callback);
      document.body.appendChild(iframe);
      return promise;
    }

    function logMessage() {
      frames[0].logMessage();
    }

    function addResource() {
      var script = document.createElement("script");
      script.src = "data:application/javascript," + escape("function test_func(){};");
      document.head.appendChild(script);
    }
  `);

  await ExtensionsTestRunner.runExtensionTests([
    function extension_testGetAllResources(nextTest) {
      function callback(resources) {
        // For some reason scripts from tests previously run in the same test shell sometimes appear, so we need to filter them out.
        var resourceURLsWhiteList = ["subframe.html", "abe.png", "audits-style1.css", "test-script.js"];
        function filter(resource) {
          for (var i = 0; i < resourceURLsWhiteList.length; ++i) {
            if (resource.url.indexOf(resourceURLsWhiteList[i]) !== -1) {
              resourceURLsWhiteList.splice(i, 1);
              return true;
            }
          }
          return false;
        }

        function compareResources(a, b) {
          return trimURL(a.url).localeCompare(trimURL(b.url));
        }
        resources = resources.filter(filter);
        resources.sort(compareResources);
        output("page resources:");
        dumpObject(Array.prototype.slice.call(arguments), { url: "url" });
      }
      invokePageFunctionAsync("loadFrame", function() {
        webInspector.inspectedWindow.getResources(callbackAndNextTest(callback, nextTest));
      });
    },

    function extension_runWithResource(regexp, callback) {
      function onResources(resources) {
        for (var i = 0; i < resources.length; ++i) {
          if (regexp.test(resources[i].url)) {
            callback(resources[i])
            return;
          }
        }
        throw "Failed to find a resource: " + regexp.toString();
      }
      webInspector.inspectedWindow.getResources(onResources);
    },

    function extension_testGetResourceContent(nextTest) {
      function onContent() {
        dumpObject(Array.prototype.slice.call(arguments));
      }
      extension_runWithResource(/test-script\.js$/, function(resource) {
        resource.getContent(callbackAndNextTest(onContent, nextTest));
      });
    },

    function extension_testSetResourceContent(nextTest) {
      evaluateOnFrontend("TestRunner.waitForStyleSheetChangedEvent(reply);", step2);

      extension_runWithResource(/audits-style1\.css$/, function(resource) {
        resource.setContent("div.test { width: 126px; height: 42px; }", false, function() {});
      });

      function step2() {
        webInspector.inspectedWindow.eval("frames[0].document.getElementById('test-div').clientWidth", function(result) {
          output("div.test width after stylesheet edited (should be 126): " + result);
          nextTest();
        });
      }
    },

    function extension_testOnContentCommitted(nextTest) {
      var expected_content = "div.test { width: 220px; height: 42px; }";

      webInspector.inspectedWindow.onResourceContentCommitted.addListener(onContentCommitted);
      extension_runWithResource(/audits-style1\.css$/, function(resource) {
        resource.setContent("div.test { width: 140px; height: 42px; }", false);
      });
      // The next step is going to produce a console message that will be logged, so synchronize the output now.
      evaluateOnFrontend("TestRunner.deprecatedRunAfterPendingDispatches(reply)", function() {
        extension_runWithResource(/abe\.png$/, function(resource) {
          resource.setContent("", true);
        });
        extension_runWithResource(/audits-style1\.css$/, function(resource) {
          resource.setContent(expected_content, true);
        });
      });

      function onContentCommitted(resource, content) {
        output("content committed for resource " + trimURL(resource.url) + " (type: " + resource.type + "), new content: " + content);
        if (!/audits-style1\.css$/.test(resource.url) || content !== expected_content)
          output("FAIL: stray onContentEdited event");
        webInspector.inspectedWindow.onResourceContentCommitted.removeListener(onContentCommitted);
        resource.getContent(function(content) {
          output("Revision content: " + content);
          nextTest();
        });
      }
    },

    function extension_testOnResourceAdded(nextTest) {
      evaluateOnFrontend("SourcesTestRunner.startDebuggerTest(reply);", step2);

      function step2() {
        webInspector.inspectedWindow.onResourceAdded.addListener(onResourceAdded);
        webInspector.inspectedWindow.eval("addResource()");
      }

      function onResourceAdded(resource) {
        if (resource.url.indexOf("test_func") === -1)
            return;
        output("resource added:");
        dumpObject(Array.prototype.slice.call(arguments), { url: "url" });
        webInspector.inspectedWindow.onResourceAdded.removeListener(onResourceAdded);

        evaluateOnFrontend("SourcesTestRunner.resumeExecution(reply);", nextTest);
      }
    },

    function extension_testOpenResourceHandler(nextTest) {
      function handleOpenResource(resource, lineNumber) {
        output("handleOpenResource() invoked [this should only appear once!]: ");
        dumpObject(Array.prototype.slice.call(arguments), { url: "url" });
        webInspector.panels.setOpenResourceHandler(null);
        evaluateOnFrontend("TestRunner.clickOnURL(); reply()", nextTest);
      }
      webInspector.panels.setOpenResourceHandler(handleOpenResource);
      webInspector.inspectedWindow.eval("logMessage()", function() {
        evaluateOnFrontend("TestRunner.clickOnURL();");
        evaluateOnFrontend("Components.Linkifier._linkHandlerSetting().set('test extension'); TestRunner.clickOnURL();");
      });
    },
  ]);
})();
