(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Tests how Target domain works with workers.');

  async function createWorker() {
    testRunner.log('Starting worker...');
    await session.evaluate(`
      if (!window.workers)
        window.workers = [];
      window.workers.push(new Worker('${testRunner.url('../resources/empty-worker.js')}'));
    `);
  }

  async function destroyWorker() {
    testRunner.log('Destroying worker...');
    await session.evaluate(`
      window.workers.shift().terminate();
    `);
  }

  var attachedWorkerId;
  dp.Target.onTargetCreated(event => testRunner.log(event, null, ['targetId', 'url', 'title']));
  dp.Target.onTargetDestroyed(event => testRunner.log(event, null, ['targetId', 'url', 'title']));
  dp.Target.onAttachedToTarget(event => {
    testRunner.log(event, null, ['targetId', 'url', 'title', 'sessionId']);
    attachedWorkerId = event.params.targetInfo.targetId;
  });
  dp.Target.onDetachedFromTarget(event => testRunner.log(event, null, ['targetId', 'url', 'title', 'sessionId']));

  testRunner.log('Getting workers');
  testRunner.log(await dp.Target.getWorkers({subscribe: false, autoAttach: false, waitForDebuggerOnStart: false}), null, ['id', 'targetId', 'url', 'title']);

  await createWorker();
  testRunner.log('Getting workers + subscribe');
  testRunner.log(await dp.Target.getWorkers({subscribe: true, autoAttach: false, waitForDebuggerOnStart: false}), null, ['id', 'targetId', 'url', 'title']);

  createWorker();
  await dp.Target.onceTargetCreated();
  testRunner.log('Getting workers + autoattach');
  var response = await dp.Target.getWorkers({subscribe: false, autoAttach: true, waitForDebuggerOnStart: false});
  var workerId = response.result.workers[0].targetId;
  testRunner.log(response, null, ['id', 'targetId', 'url', 'title']);

  testRunner.log('Attaching to worker');
  await dp.Target.attachToTarget({targetId: workerId});

  testRunner.log('Detaching from worker');
  await dp.Target.detachFromTarget({targetId: workerId});

  await destroyWorker();
  createWorker();
  await dp.Target.onceAttachedToTarget();

  testRunner.log('Detaching from worker');
  await dp.Target.detachFromTarget({targetId: attachedWorkerId});

  testRunner.log('Getting workers + autoattach + subscribe');
  testRunner.log(await dp.Target.getWorkers({subscribe: true, autoAttach: true, waitForDebuggerOnStart: false}), null, ['id', 'targetId', 'url', 'title']);
  createWorker();
  await dp.Target.onceTargetCreated();
  await dp.Target.onceAttachedToTarget();

  testRunner.log('Detaching from worker');
  await dp.Target.detachFromTarget({targetId: attachedWorkerId});

  testRunner.log('Getting workers');
  testRunner.log(await dp.Target.getWorkers({subscribe: true, autoAttach: true, waitForDebuggerOnStart: false}), null, ['id', 'targetId', 'url', 'title']);

  destroyWorker();
  await dp.Target.onceTargetDestroyed();
  destroyWorker();
  await dp.Target.onceTargetDestroyed();
  destroyWorker();
  await dp.Target.onceTargetDestroyed();

  testRunner.log('Getting workers');
  testRunner.log(await dp.Target.getWorkers({subscribe: false, autoAttach: false, waitForDebuggerOnStart: false}), null, ['id', 'targetId', 'url', 'title']);

  testRunner.completeTest();
})
