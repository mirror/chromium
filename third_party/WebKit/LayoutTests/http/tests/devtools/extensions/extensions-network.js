// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests WebInspector extension API\n`);
  await TestRunner.loadModule('extensions_test_runner');
  await TestRunner.loadHTML(`
      <style>
      @font-face {
          font-family: 'test';
          src: url(${TestRunner.url('../../resources/Ahem.ttf')});
      }

      p { font-family: 'test'; }
      </style>
      <img src="${TestRunner.url('resources/abe.png')}">
      <img src="${TestRunner.url('resources/missing-image.png')}">
      <script>
      function doXHR() {
        return fetch('${TestRunner.url('.')}');
      }
      </script>
      <p>some text.</p>
  `);
  await TestRunner.addStylesheetTag('resources/audits-style1.css');
  ExtensionsTestRunner.runExtensionTests([
    function extension_testGetHAR(nextTest) {
      function compareEntries(a, b) {
        return a.request.url.toLowerCase().localeCompare(b.request.url.toLowerCase());
      }

      function onHAR(result) {
        result.entries.sort(compareEntries);

        for (var i = 0; i < result.entries.length; ++i)
            output("resource: " + result.entries[i].request.url);
      }
      extension_doXHR(function() {
        webInspector.network.getHAR(callbackAndNextTest(onHAR, nextTest));
      });
    },

    function extension_doXHR(callback) {
      invokePageFunctionAsync("doXHR", callback);
    },

    function extension_testRequestNotification(nextTest) {
      function onRequestFinished(request) {
          output("Request finished: " + request.request.url.replace(/.*((\/[^/]*){3}$)/,"...$1"));
      }

      webInspector.network.onRequestFinished.addListener(callbackAndNextTest(onRequestFinished, nextTest));
      extension_doXHR();
    },

    function extension_onRequestBody(content, encoding) {
      dumpObject(Array.prototype.slice.call(arguments));
    },

    function extension_testGetRequestContent(nextTest) {
      extension_getRequestByUrl([/audits-style1.css$/], function(request) {
        request.getContent(callbackAndNextTest(extension_onRequestBody, nextTest));
      });
    },

    function extension_testGetResourceContentEncoded(nextTest) {
      extension_getRequestByUrl([/abe.png$/ ], function(request) {
        request.getContent(callbackAndNextTest(extension_onRequestBody, nextTest));
      });
    }
  ]);
})();
