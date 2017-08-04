(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      'Test that page performance metrics are retrieved.');

  await dumpMetrics();
  await dumpMetrics(['NodeCount', 'DocumentCount'], 3);
  await dumpMetrics(['NodeCount']);
  await dumpMetrics([]);

  async function dumpMetrics(names, times) {
    if (names)
      await dp.Performance.enableMetrics({metrics: names});
    for (let i of new Array(times || 1)) {
      const {result:{metrics}} = await dp.Performance.getMetrics();
      testRunner.log(JSON.stringify(metrics.map(metric => metric.name)));
    }
  }

  testRunner.completeTest();
})
