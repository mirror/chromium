(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests presence of the request body for original request and its clone.`);

  await dp.Network.enable();

  async function reportRequest() {
    var request = await dp.Network.onceRequestWillBeSent();
    testRunner.log(request.params.request.postData);
  }

  session.evaluate(`
    var r1 = new Request('/anypage', { method: 'post', body: 'request body'});
    var r2 = r1.clone();
    fetch(r1).then(() => fetch(r2));
  `);
  await reportRequest();
  await reportRequest();

  testRunner.completeTest();
})
