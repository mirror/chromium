// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Test that computing the initiator graph works.\n`);
  await TestRunner.loadModule('network_test_runner');
  await TestRunner.showPanel('network');
  await TestRunner.evaluateInPagePromise(`
      function makeFetchInWorker(url)
      {
          return new Promise((resolve) => {
              var worker = new Worker('/devtools/network/resources/fetch-worker.js');
              worker.onmessage = (event) => {
                  resolve(JSON.stringify(event.data));
              };
              worker.postMessage(url);
          });
      }
  `);

  NetworkTestRunner.recordNetwork();

  TestRunner.callFunctionInPageAsync('makeFetchInWorker', [{url: './empty.html'}])
      .then((result) => {
        TestRunner.addResult('Fetch in worker result: ' + result);
        var requests = NetworkTestRunner.networkRequests();
        requests.forEach((request) => {
          TestRunner.addResult('\n' + request.url());
          var graph = NetworkLog.networkLog.initiatorGraphForRequest(request);
          TestRunner.addResult('Initiators ' + Array.from(graph.initiators).map(request => request._url));
          TestRunner.addResult('Initiated ' + Array.from(graph.initiated).map(request => request._url));
        });
        TestRunner.completeTest();
      });
})();
