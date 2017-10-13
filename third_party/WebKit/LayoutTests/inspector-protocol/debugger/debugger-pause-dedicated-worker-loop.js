(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Tests that worker can be interrupted and paused.');

  await session.evaluate(`
    window.worker = new Worker('${testRunner.url('resources/dedicated-worker-loop.js')}');
    var resolve;
    window.workerMessageReceivedPromise = new Promise(f => resolve = f);
    window.worker.onmessage = function(event) {
      if (event.data === 'WorkerMessageReceived')
        resolve();
    };
  `);
  testRunner.log('Started worker');

  var workerId;
  var workerRequestId = 1;
  function sendCommandToWorker(method, params) {
    var message = {method, params, id: workerRequestId};
    dp.Target.sendMessageToTarget({targetId: workerId, message: JSON.stringify(message)});
    return workerRequestId++;
  }

  async function attachToWorker() {
    var response = await dp.Target.getWorkers({subscribe: true, autoAttach: false, waitForDebuggerOnStart: false});
    var workerId;
    if (response.result.workers.length) {
      workerId = response.result.workers[0].targetId;
    } else {
      response = await dp.Target.onceTargetCreated(event => event.params.targetInfo.type === 'worker');
      workerId = response.params.targetInfo.targetId;
    }
    testRunner.log('Worker created');
    await dp.Target.attachToTarget({targetId: workerId});
    return workerId;
  }

  var debuggerEnableRequestId = -1;
  var evaluateRequestId = -1;

  dp.Target.onReceivedMessageFromTarget(async messageObject => {
    var message = JSON.parse(messageObject.params.message);
    if (message.id === debuggerEnableRequestId) {
      testRunner.log('Did enable debugger');
      // Start tight loop in the worker.
      await dp.Runtime.evaluate({expression: 'worker.postMessage(1)' });
      testRunner.log('Did post message to worker');
    }
    if (message.id === evaluateRequestId) {
      var value = message.result.result.value;
      if (value === true)
        testRunner.log('SUCCESS: evaluated, result: ' + value);
      else
        testRunner.log('FAIL: evaluated, result: ' + value);
      testRunner.completeTest();
    }
  });

  workerId = await attachToWorker();
  testRunner.log('didConnectToWorker');
  // Enable debugger so that V8 can interrupt and handle inspector commands while there is a script running in a tight loop.
  debuggerEnableRequestId = sendCommandToWorker('Debugger.enable', {});

  await session.evaluateAsync('workerMessageReceivedPromise');
  evaluateRequestId = sendCommandToWorker('Runtime.evaluate', { 'expression': 'message_id > 1'});
})
