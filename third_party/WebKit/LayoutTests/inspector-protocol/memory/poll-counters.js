(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      'Test that page counters are retrieved.');

  await dp.Memory.enableCounters({counters: ['NodeCounter', 'FrameCounter']});
  await pollCounters();
  await pollCounters();

  await dp.Memory.disableCounters({counters: ['FrameCounter']});
  await pollCounters();

  async function pollCounters() {
    const {result:{counters}} = await dp.Memory.pollCounters();
    testRunner.log(JSON.stringify(Object.keys(counters)));
  }

  testRunner.completeTest();
})
