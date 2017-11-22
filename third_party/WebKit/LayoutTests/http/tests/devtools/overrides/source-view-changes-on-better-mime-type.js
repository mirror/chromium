// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(
      `When overrides enabled and sourceframe has no known resource type, uses network resource/mime-type if avaialbe.\n`);
  await TestRunner.loadModule('bindings_test_runner');
  await TestRunner.loadModule('sources_test_runner');

  var project = await BindingsTestRunner.createOverrideProject('file:///tmp');
  BindingsTestRunner.setOverridesEnabled(true);
  Persistence.networkPersistenceManager.addFileSystemOverridesProject('127.0.0.1:8000', project);
  await TestRunner.showPanel('sources');

  project.createFile('127.0.0.1%3a8000/devtools/resources/serve-image.php', atob('R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw=='));

  var fileSystemUISourceCode = await TestRunner.waitForUISourceCode('serve-image.php');
  if (!fileSystemUISourceCode) {
    TestRunner.addResult('Error: No UISourceCode found for serve-image.php.');
    return;
  }

  var fileSystemFrame = await SourcesTestRunner.showUISourceCodePromise(fileSystemUISourceCode);

  dumpFrameTypeInfo(fileSystemFrame);

  await TestRunner.loadHTML(`<img src="http://127.0.0.1:8000/devtools/resources/serve-image.php" />`);

  var networkUISourceCode = await TestRunner.waitForUISourceCode('http://127.0.0.1:8000/devtools/resources/serve-image.php');

  var networkFrame = await SourcesTestRunner.showUISourceCodePromise(fileSystemUISourceCode);
  dumpFrameTypeInfo(networkFrame);

  /**
   * @param {!SourceFrame.SourceFrame}
   */
  function dumpFrameTypeInfo(frame) {
    TestRunner.addResult('Frame Url: ' + frame.uiSourceCode().url());
    if (frame instanceof Sources.JavaScriptSourceFrame) {
      TestRunner.addResult('Frame type: Sources.JavaScriptSourceFrame');
      return;
    }
    if (frame instanceof SourceFrame.ImageView) {
      TestRunner.addResult('Frame type: SourceFrame.ImageView');
      return;
    }
    if (frame instanceof SourceFrame.FontView) {
      TestRunner.addResult('Frame type: SourceFrame.FontView');
      return;
    }
    if (frame instanceof Sources.UISourceCodeFrame) {
      TestRunner.addResult('Frame type: Sources.UISourceCodeFrame');
      return;
    }
    TestRunner.addResult('Frame type: Unknown');
  }



  var uiSourceCode = Workspace.workspace.uiSourceCodes().find(uiSourceCode => uiSourceCode.url().endsWith('cross-origin-iframe.html'));
  if (!uiSourceCode)
    throw "No uiSourceCode.";
  var uiSourceCodeFrame = new Sources.UISourceCodeFrame(uiSourceCode);
  TestRunner.addResult('URL: ' + uiSourceCode.url().substr(uiSourceCode.url().lastIndexOf('/') + 1));
  TestRunner.addResult('Can Edit Source: ' + uiSourceCodeFrame._canEditSource());
  TestRunner.completeTest();
})();
