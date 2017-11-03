(async function(testRunner) {
  var {page, session, dp} = await testRunner.startHTML(`<div>not changed.</div>`,
      'Tests that Page.setDocumentContent returns new loaderId for every call.');

  await dp.Page.enable();

  var frameTreeResponse = await dp.Page.getFrameTree();
  var mainFrameId = frameTreeResponse.result.frameTree.frame.id;

  var response = await dp.Page.setDocumentContent({
    frameId: mainFrameId,
    html: '<div>CHANGED-1</div>',
  });
  var loaderId1 = response.result.loaderId;

  var response = await dp.Page.setDocumentContent({
    frameId: mainFrameId,
    html: '<div>CHANGED-1</div>',
  });
  var loaderId2 = response.result.loaderId;
  testRunner.log('loaders are different: ' + (loaderId1 !== loaderId2));

  testRunner.completeTest();
})
