(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests encoding of a response body`);

  await dp.Network.enable();

  async function logResponse(url, encoding, quality, sizeOnly) {
    testRunner.log(`\n\nResults for ${url} encoding=${encoding} q=${quality} sizeOnly=${sizeOnly}`);
    var responseReceivedFuture = dp.Network.onceResponseReceived();

    session.evaluate(`fetch(${JSON.stringify(url)})`);

    const requestId = (await responseReceivedFuture).params.requestId;
    const result = (await dp.Audits.getEncodedResponse({requestId, encoding, quality, sizeOnly})).result;

    testRunner.log(`original=${result.originalSize} encoded=${result.encodedSize}`);
    testRunner.log(`body=${result.body}`);
  }

  await logResponse("/resources/square200.png", "jpeg", .8);
  await logResponse("/resources/square200.png", "webp", .8, false);
  await logResponse("/resources/square200.png", "jpeg", 1, true);
  await logResponse("/resources/square200.png", "jpeg", .5, true);
  await logResponse("/resources/square20.bmp", "jpeg", .8, true);

  testRunner.completeTest();
})
