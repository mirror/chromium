// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`\n`);
  await TestRunner.showPanel('network');
  await TestRunner.loadHTML(`
    Tests that responseReceived is called on NetworkDispatcher for resource requests denied due to X-Frame-Options header.
    `);
  await TestRunner.evaluateInPagePromise(`
      function loadIFrameWithDownload()
      {
          var iframe = document.createElement("iframe");
          iframe.setAttribute("src", "http://127.0.0.1:8000/security/XFrameOptions/resources/x-frame-options-deny.cgi");
          document.body.appendChild(iframe);
      }
  `);

  TestRunner.addSniffer(SDK.NetworkDispatcher.prototype, 'responseReceived', responseReceived);
  TestRunner.addSniffer(SDK.NetworkDispatcher.prototype, 'loadingFailed', loadingFailed);
  TestRunner.addSniffer(SDK.NetworkDispatcher.prototype, 'loadingFinished', loadingFinished);
  TestRunner.evaluateInPage('loadIFrameWithDownload()');

  function responseReceived(requestId, time, resourceType, response) {
    var request = NetworkLog.networkLog.requestByManagerAndId(TestRunner.networkManager, requestId);
    if (/x-frame-options-deny\.cgi/.exec(request.url())) {
      TestRunner.addResult('Received response for x-frame-options-deny.cgi');
      TestRunner.addResult('SUCCESS');
      TestRunner.completeTest();
    }
  }

  function loadingFinished(requestId, finishTime) {
    var request = NetworkLog.networkLog.requestByManagerAndId(TestRunner.networkManager, requestId);
    if (/x-frame-options-deny\.cgi/.exec(request.url()))
      TestRunner.completeTest();
  }

  function loadingFailed(requestId, time, localizedDescription, canceled) {
    var request = NetworkLog.networkLog.requestByManagerAndId(TestRunner.networkManager, requestId);
    if (/x-frame-options-deny\.cgi/.exec(request.url())) {
      TestRunner.addResult('TODO(mkwst): This started failing when we moved XFO to the browser.');
      TestRunner.completeTest();
    }
  }
})();
