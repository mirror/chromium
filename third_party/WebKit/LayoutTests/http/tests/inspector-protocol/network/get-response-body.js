(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests fetching of a response body.`);

  await dp.Network.enable();

  async function logResponseBody(url) {
    var requestWillBeSentFuture = dp.Network.onceRequestWillBeSent();
    var responseReceivedFuture = dp.Network.onceResponseReceived();

    session.evaluate(`fetch(${JSON.stringify(url)});`);

    var requestWillBeSent = (await requestWillBeSentFuture).params;
    var responseReceived = (await responseReceivedFuture).params;
    testRunner.log(`Request for ${requestWillBeSent.request.url}`);
    var data = await dp.Network.getResponseBody({requestId: requestWillBeSent.requestId});
    testRunner.log(data.result.body);
  }

  await logResponseBody(testRunner.url('./resources/final.js'));
  await logResponseBody(testRunner.url('./resources/test.css'));

  testRunner.completeTest();
})
