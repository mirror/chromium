(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests interception blocking, modification of network fetches.`);

  var InterceptionHelper = await testRunner.loadScript('../resources/interception-test.js');
  var helper = new InterceptionHelper(testRunner, session);

  var headersMaskList = new Set(['date', 'server', 'last-modified', 'etag', 'keep-alive']);

  var requestInterceptedDict = {
    'simple-iframe.html': event => {
      testRunner.log('Request Intercepted: ' + event.params.request.url.split('/').pop());
      testRunner.log('  responseCode: ' + event.params.responseCode);
      testRunner.log('  responseHeaders:');
      for (var headerName of Object.keys(event.params.responseHeaders)) {
        var headerValue = event.params.responseHeaders[headerName];
        if (headersMaskList.has(headerName.toLowerCase()))
          headerValue = '<Masked>';
        testRunner.log(`    ${headerName}: ${headerValue}`);
      }
      testRunner.log('  responseBody:');
      testRunner.log(atob(event.params.responseBody));
      helper.modifyRequest(event, {rawResponse: btoa('<html>\n<body>This content was rewritten!</body>\n</html>')});
      testRunner.log('');
    }
  };

  await helper.startInterceptionTest(requestInterceptedDict, Infinity, true);

  var requestId = '';
  session.protocol.Network.onRequestWillBeSent(event => {
    testRunner.log('RequestWillBeSent Event fired for url: ' + event.params.request.url);
    if (requestId)
      throw "requestId already set";
    requestId = event.params.requestId;
  });
  await new Promise(resolve => {
    session.protocol.Network.onResponseReceived(event => {
      testRunner.log('ResponseReceived Event fired');
      resolve();
    });
    session.evaluate(`
      var iframe = document.createElement('iframe');
      iframe.src = '${testRunner.url('./resources/simple-iframe.html')}';
      document.body.appendChild(iframe);
    `);
  });

  var result = await session.protocol.Network.getResponseBody({requestId: requestId});
  testRunner.log('Body content received by renderer:');
  testRunner.log(result.result.base64Encoded ? atob(result.result.body) : result.result.body);

  testRunner.completeTest();
})
