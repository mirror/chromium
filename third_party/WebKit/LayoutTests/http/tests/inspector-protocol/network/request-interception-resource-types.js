(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests interception of specified resource types.`);

  await session.protocol.Network.enable();
  testRunner.log('Network agent enabled');
  await session.protocol.Page.enable();
  testRunner.log('Page agent enabled');

  var numFramesLoaded = 0;

  session.protocol.Page.onFrameStoppedLoading(async event => {
    testRunner.log('Page.FrameStoppedLoading\n');
    numFramesLoaded ++;
    if (numFramesLoaded == 1) {
      testRunner.log('Intercept stylesheets only');
      await session.protocol.Network.setRequestInterceptionEnabled({"enabled": true, resourceTypes: ["Stylesheet"]});
      session.evaluate(`
        var iframe = document.createElement('iframe');
        iframe.src = '${testRunner.url('./resources/resource-iframe.html')}';
        document.body.appendChild(iframe);
      `);
    } else {
      testRunner.completeTest();
    }
  });

  session.protocol.Network.onRequestIntercepted(async event => {
    var filename = event.params.request.url.split('/').pop();
    testRunner.log('Request Intercepted: ' + filename);
    session.protocol.Network.continueInterceptedRequest({interceptionId: event.params.interceptionId, errorReason: 'AddressUnreachable'});
  });
  testRunner.log('Intercept scripts only');
  await session.protocol.Network.setRequestInterceptionEnabled({"enabled": true, resourceTypes: ["Script"]});
  session.evaluate(`
    var iframe = document.createElement('iframe');
    iframe.src = '${testRunner.url('./resources/resource-iframe.html')}';
    document.body.appendChild(iframe);
  `);
})
