(async function(testRunner) {
  let {page, session, dp} = await testRunner.startBlank('Tests sendMessage with invalid message.');

  await dp.Target.setAutoAttach({autoAttach: true, waitForDebuggerOnStart: false});
  session.evaluate(`
    window.worker = new Worker('${testRunner.url('../resources/worker.js')}');
  `);
  let {params:{sessionId}} = await dp.Target.onceAttachedToTarget();

  testRunner.log('JSON syntax error..');
  dp.Target.sendMessageToTarget({
    sessionId: sessionId,
    message: "{\"method\":\"Runtime.evaluate\",\"params\":{\"expression\":\"1\", awaitPromise: true},\"id\":1}"
  });
  let {params:{message}} = await dp.Target.onceReceivedMessageFromTarget();
  testRunner.log(message);

  testRunner.log('JSON with primitive value..');
  dp.Target.sendMessageToTarget({
    sessionId: sessionId,
    message: "42"
  });
  ({params:{message}} = await dp.Target.onceReceivedMessageFromTarget());
  testRunner.log(message);

  testRunner.log('JSON without method property..');
  dp.Target.sendMessageToTarget({
    sessionId: sessionId,
    message: "{}"
  });
  ({params:{message}} = await dp.Target.onceReceivedMessageFromTarget());
  testRunner.log(message);

  testRunner.log('JSON without id property..');
  dp.Target.sendMessageToTarget({
    sessionId: sessionId,
    message: "{\"method\":\"Runtime.evaluate\",\"params\":{\"expression\":\"42\"}}"
  });
  ({params:{message}} = await dp.Target.onceReceivedMessageFromTarget());
  testRunner.log(message);

  testRunner.log('Valid JSON..');
  dp.Target.sendMessageToTarget({
    sessionId: sessionId,
    message: "{\"method\":\"Runtime.evaluate\",\"params\":{\"expression\":\"42\"},\"id\":1}"
  });
  ({params:{message}} = await dp.Target.onceReceivedMessageFromTarget());
  testRunner.log(message);
  testRunner.completeTest();
})
