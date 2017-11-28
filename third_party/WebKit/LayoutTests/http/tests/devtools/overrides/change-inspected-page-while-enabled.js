// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Ensures if navigating away from a project that already exists to a page without a project then a project gets created it works properly.\n`);
  await TestRunner.loadModule('bindings_test_runner');
  await TestRunner.loadModule('sources');

  var overridesNavigatorView = new Sources.OverridesNavigatorView();

  await TestRunner.navigatePromise('http://127.0.0.1:8000/devtools/network/resources/empty.html');
  var {isolatedFileSystem} = await BindingsTestRunner.createOverrideProject('file:///tmp/folder1');
  overridesNavigatorView._overridesFileSystemCreated(isolatedFileSystem);

  await TestRunner.navigatePromise('http://localhost:8000/devtools/network/resources/empty.html');
  var {isolatedFileSystem, testFileSystem} = await BindingsTestRunner.createOverrideProject('file:///tmp/folder2');
  overridesNavigatorView._overridesFileSystemCreated(isolatedFileSystem);

  var uiSourceCode = Workspace.workspace.uiSourceCodeForURL('http://localhost:8000/devtools/network/resources/empty.html');
  uiSourceCode.setContent('This is a test.', false);

  TestRunner.addSniffer(
      Persistence.networkPersistenceManager, '_fileCreatedForTest',
      () => {
        TestRunner.addResult(testFileSystem.dumpAsText());
        TestRunner.completeTest();
      }, false);

  async function createFileSystem(fileSystemPath) {
    var testFileSystem = new BindingsTestRunner.TestFileSystem(fileSystemPath);
    var isolatedFileSystem = await testFileSystem.reportCreatedPromise();
    isolatedFileSystem._type = 'overrides';
    return [isolatedFileSystem, testFileSystem];
  }
})();
