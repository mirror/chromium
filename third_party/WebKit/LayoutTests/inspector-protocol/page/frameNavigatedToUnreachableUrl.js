(async function(testRunner) {
  let {page, session, dp} = await testRunner.startBlank('');

  dp.Page.enable();
  session.evaluate(`
    var frame = document.createElement('iframe');
    frame.src = '${testRunner.url('../resources/idont_exist.html')}';
    document.body.appendChild(frame);
  `);
  let result = await dp.Page.onceFrameNavigated();
  testRunner.log('Page navigated, url = ' + result.params.frame.url);
  testRunner.log('UnreachableUrl = ' +
      result.params.frame.unreachableUrl.split('/').pop());
  testRunner.completeTest();
})
