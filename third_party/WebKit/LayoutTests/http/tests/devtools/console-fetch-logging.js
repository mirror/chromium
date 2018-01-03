// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `Tests that fetch logging works when XMLHttpRequest Logging is Enabled and doesn't show logs when it is Disabled.\n`);
  await TestRunner.loadModule('console_test_runner');
  await TestRunner.loadModule('network_test_runner');
  await TestRunner.evaluateInPagePromise(`
      function requestHelper(method, url, callback, verbose)
      {
          verbose && console.log("sending a " + method + " request to " + url);
          // Delay invoking callback to let didFinishLoading() a chance to fire and log
          // console message.
          function delayCallback()
          {
              setTimeout(callback, 0);
          }
          makeFetch(url, {method: method}).then(delayCallback);
      }

      function makeRequests(verbose)
      {
          var callback;
          var promise = new Promise((fulfill) => callback = fulfill);
          step1();
          return promise;

          function step1()
          {
              // Page that exists.
              requestHelper("GET", "resources/xhr-exists.html", step2, verbose);
          }

          function step2()
          {
              // Page that doesn't exist.
              requestHelper("GET", "resources/xhr-does-not-exist.html", step3, verbose);
          }

          function step3()
          {
              // POST to a page.
              requestHelper("POST", "resources/post-target.cgi", step4, verbose);
          }

          function step4()
          {
              // (Failed) cross-origin request
              requestHelper("GET", "http://localhost:8000/devtools/resources/xhr-exists.html", callback, verbose);
          }
      }
  `);

  step1();

  async function step1() {
    Common.settingForTest('monitoringXHREnabled').set(true);
    await TestRunner.callFunctionInPageAsync('makeRequests');

    TestRunner.deprecatedRunAfterPendingDispatches(() => {
      TestRunner.addResult('Fetch with logging enabled: ');
      // Sorting console messages to prevent flakiness.
      TestRunner.addResults(ConsoleTestRunner.dumpConsoleMessagesIntoArray().sort());
      Console.ConsoleView.clearConsole();
      step2();
    });
  }

  async function step2() {
    Common.settingForTest('monitoringXHREnabled').set(false);
    await TestRunner.callFunctionInPageAsync('makeRequests', [/* verbose */ true]);

    TestRunner.deprecatedRunAfterPendingDispatches(() => {
      TestRunner.addResult('\n==========================\n');
      TestRunner.addResult('Fetch with logging disabled: ');
      ConsoleTestRunner.dumpConsoleMessages();
      TestRunner.completeTest();
    });
  }
})();
