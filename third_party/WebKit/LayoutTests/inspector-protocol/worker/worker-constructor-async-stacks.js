(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Async stack trace in worker constructor.');
  let debuggers = new Map();

  await dp.Target.setAutoAttach({autoAttach: true, waitForDebuggerOnStart: true});

  testRunner.log('Setup page session');
  dp.Debugger.enable();
  let pageAsyncToken = (await dp.Debugger.setAsyncCallStackDepth({maxDepth: 32})).result.asyncToken;
  debuggers.set(pageAsyncToken, dp.Debugger);
  testRunner.log('Set breakpoint before worker created');
  dp.Debugger.setBreakpointByUrl({
    url: 'test.js',
    lineNumber: 2,
    columnNumber: 13
  });
  session.evaluate(`
var blob = new Blob(['console.log(239);//# sourceURL=worker.js'], {type: 'application/javascript'});
var worker = new Worker(URL.createObjectURL(blob));
//# sourceURL=test.js`);

  testRunner.log('Run stepInto with breakOnAsyncCall flag');
  await dp.Debugger.oncePaused();
  dp.Debugger.stepInto({breakOnAsyncCall: true});
  testRunner.log('Get scheduledAsyncTaskId');
  let {params:{scheduledAsyncTaskId}} = await dp.Debugger.oncePaused();
  dp.Debugger.resume();

  testRunner.log('Setup worker session');
  let {params:{sessionId}} = await dp.Target.onceAttachedToTarget();
  let wc = new WorkerProtocol(dp, sessionId);

  wc.dp.Debugger.enable();
  let workerAsyncToken = (await wc.dp.Debugger.setAsyncCallStackDepth({maxDepth: 32})).asyncToken;
  debuggers.set(workerAsyncToken, wc.dp.Debugger);
  testRunner.log('Request pause on async task and run worker');
  wc.dp.Debugger.pauseOnAsyncTask({
    asyncTaskId: scheduledAsyncTaskId,
    sourceAsyncToken: pageAsyncToken
  });
  wc.dp.Runtime.runIfWaitingForDebugger();

  let {callFrames, remoteAsyncStackTrace} = await wc.dp.Debugger.oncePaused();
  logCallFrames(callFrames);

  let {result:{stackTrace}} = await debuggers.get(remoteAsyncStackTrace.asyncToken).getStackTrace({
    asyncTaskId: remoteAsyncStackTrace.asyncTaskId
  });
  testRunner.log('--' + stackTrace.description + '--');
  logCallFrames(stackTrace.callFrames);
  testRunner.completeTest();

  function logCallFrames(callFrames) {
    for (let frame of callFrames) {
      let functionName = frame.functionName || '(anonymous)';
      let url = frame.url;
      let location = frame.location || frame;
      testRunner.log(functionName + ' at ' + url + ':' + location.lineNumber + ':' + location.columnNumber);
    }
  }
})
