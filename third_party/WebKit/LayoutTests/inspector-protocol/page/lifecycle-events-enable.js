(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank('Test lifecycle events');

  await Promise.all([
    waitForLifecycleEvents(['DOMContentLoaded', 'load', 'networkIdle', 'networkAlmostIdle']),
    dp.Page.enable()
  ]);
  testRunner.log('Received initial lifecycle events.');
  await dp.Page.disable();

  await Promise.all([
    waitForLifecycleEvents(['DOMContentLoaded', 'load', 'networkIdle', 'networkAlmostIdle']),
    dp.Page.enable()
  ]);
  testRunner.log('Received lifecycle events after reenabling.');
  testRunner.completeTest();

  async function waitForLifecycleEvents(names) {
    const pendingEvents = new Set(names);
    while (pendingEvents.size) {
      var event = await dp.Page.onceLifecycleEvent();
      pendingEvents.delete(event.params.name);
    }
  }
})
