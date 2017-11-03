(async function(testRunner) {
  var {page, session, dp} = await testRunner.startHTML(`<div>Привет мир</div>`,
      'Tests that Page.setDocumentContent works.');

  await dp.Page.enable();
  testRunner.log('Page enabled');

  var frameTreeResponse = await dp.Page.getFrameTree();
  var mainFrameId = frameTreeResponse.result.frameTree.frame.id;
  testRunner.log('Main Frame obtained');

  testRunner.log('Document content before: ' + await session.evaluate(() => document.documentElement.outerHTML));
  await dp.Page.setDocumentContent({
    frameId: mainFrameId,
    html: '<div>こんにちは世界</div>',
    url: 'https://google.com'
  });
  // Wait for navigation to happen.
  await dp.Page.onceFrameNavigated();
  testRunner.log('Document content set');
  testRunner.log('Document content after: ' + await session.evaluate(() => document.documentElement.outerHTML));
  testRunner.log('    Document URL after: ' + await session.evaluate(() => window.location.href));

  testRunner.completeTest();
})
