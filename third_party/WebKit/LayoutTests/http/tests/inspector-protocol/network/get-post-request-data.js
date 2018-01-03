(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests fetching POST request body.`);

  function SendRequest(method, body) {
    return session.evaluateAsync(`
        new Promise(resolve => {
          const req = new XMLHttpRequest();
          req.open('${method}', '/', true);
          req.setRequestHeader('content-type', 'application/octet-stream');
          req.onreadystatechange = () => resolve();
          req.send(${body});
        });`);
  }

  async function SendAndReportRequest(method, body = '') {
    var requestWillBeSentPromise = dp.Network.onceRequestWillBeSent();
    await SendRequest(method, body);
    var notification = (await requestWillBeSentPromise).params;
    var request = notification.request;
    testRunner.log(`Data included: ${request.postData !== undefined}, has post data: ${request.hasPostData}`);
    try {
      var bodyResult = (await dp.Network.getPostRequestData({ requestId: notification.requestId })).result;
      testRunner.log(`Data length: ${bodyResult.contents.length}`);
    } catch (e) {
      testRunner.log('No POST data was read');
    }
  }

  await dp.Network.enable({ maxRequestWillBeSentBodySize: 512 });
  await SendAndReportRequest('POST', 'new Uint8Array(1024)');
  await SendAndReportRequest('POST', 'new Uint8Array(128)');
  await SendAndReportRequest('POST', '');
  await SendAndReportRequest('GET', 'new Uint8Array(1024)');
  testRunner.completeTest();
})
