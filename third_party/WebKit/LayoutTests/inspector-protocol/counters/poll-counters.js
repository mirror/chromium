(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      'Test that page counters are retrieved.');

  await dp.Counters.enableCounters({counters: ['NodeCounter', 'FrameCounter']});
  await dumpCounters();
  await dumpCounters();

  await dp.Counters.enableCounters({counters: ['FrameCounter']});
  await dumpCounters();

  await dp.Counters.enableCounters({counters: []});
  await dumpCounters();

  async function dumpCounters() {
    const {result:{counters}} = await dp.Counters.getCounters();
    testRunner.log(JSON.stringify(counters.map(counter => counter.name)));
  }

  testRunner.completeTest();
})
