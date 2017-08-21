(async function(testRunner) {
  testRunner.log('Tests that multiple cannot emulate simultaneously.');
  var page = await testRunner.createPage();
  var session1 = await page.createSession();
  var session2 = await page.createSession();

  async function dumpCanEmulate() {
    testRunner.log('session1.canEmulate: ' + (await session1.protocol.Emulation.canEmulate()).result.result);
    testRunner.log('session2.canEmulate: ' + (await session2.protocol.Emulation.canEmulate()).result.result);
  }

  testRunner.log('Emulating in 1');
  testRunner.logMessage(await session1.protocol.Emulation.setDeviceMetricsOverride({width: 200, height: 300, deviceScaleFactor: 0, mobile: false}));
  await dumpCanEmulate();

  testRunner.log('Emulating in 2');
  testRunner.logMessage(await session2.protocol.Emulation.setDeviceMetricsOverride({width: 200, height: 300, deviceScaleFactor: 0, mobile: false}));
  await dumpCanEmulate();

  testRunner.log('Clearing in 1');
  testRunner.logMessage(await session1.protocol.Emulation.clearDeviceMetricsOverride());
  await dumpCanEmulate();

  testRunner.log('Emulating in 2');
  testRunner.logMessage(await session2.protocol.Emulation.setDeviceMetricsOverride({width: 200, height: 300, deviceScaleFactor: 0, mobile: false}));
  await dumpCanEmulate();

  testRunner.log('Emulating in 1');
  testRunner.logMessage(await session1.protocol.Emulation.setDeviceMetricsOverride({width: 200, height: 300, deviceScaleFactor: 0, mobile: false}));
  await dumpCanEmulate();

  testRunner.log('Clearing in 2');
  testRunner.logMessage(await session2.protocol.Emulation.clearDeviceMetricsOverride());
  await dumpCanEmulate();

  testRunner.completeTest();
})
