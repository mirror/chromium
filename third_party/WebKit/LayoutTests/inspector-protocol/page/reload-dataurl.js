(async function(testRunner) {
  var dataURL = 'data:text/html,hello!';
  var {page, session, dp} = await testRunner.startURL(dataURL, 'Tests reloading pages with data URLs.');

  await dp.Page.enable();
  await dp.Page.reload({
    ignoreCache: true,
    scriptToEvaluateOnLoad: 'window.foo = 42;'
  });
  dp.Page.setLifecycleEventsEnabled({enabled: true});
  await dp.Page.onceLifecycleEvent(event => event.params.name === 'load');

  testRunner.log('Querying window.foo after reload: ' + (await session.evaluate(() => window.foo)));
  testRunner.completeTest();
})
