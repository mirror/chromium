(async function(testRunner) {
  let {page, session, dp} = await testRunner.startBlank('');

  dp.Page.enable();
  session.evaluate(`
    var frame = document.createElement('iframe');
    frame.src = '${testRunner.url('../resources/idont_exist.html')}';
    document.body.appendChild(frame);
  `);
  await dp.Page.onceFrameStartedLoading();
  testRunner.log('Started loading');
  let result = await dp.Page.onceFrameStoppedLoading();
  testRunner.log('Stopped loading, unreachableUrl = ' +
      result.params.unreachableUrl.split('/').pop());
  testRunner.completeTest();
})
