(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests that Page.lifecycleEvent is issued for important events.`);
  dp.Page.enable();
  dp.Runtime.enable();

  var events = [];
  dp.Page.onLifecycleEvent(result => {
    events.push(result.params.name);
    if (events.includes('network0quiet') && events.includes('network2quiet')) {
      events.sort();
      testRunner.log(events);
      testRunner.completeTest();
    }
  });

  dp.Page.navigate({url: testRunner.url('../resources/image.html')});
})
