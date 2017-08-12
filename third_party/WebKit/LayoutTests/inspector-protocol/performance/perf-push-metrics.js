(async function(testRunner) {
  const {page, session, dp} = await testRunner.startBlank(
      'Test that console.timeStamp generates Performance.metrics event');

  dp.Performance.onMetrics(response => {
    const metrics = response.params.metrics;
    testRunner.log('Metrics received:');
    testRunner.log(JSON.stringify(metrics.map(metric => metric.name)));
    testRunner.completeTest();
  });

  await dp.Performance.enable();
  await session.evaluate('console.timeStamp()');
  await dp.Performance.disable();
})
