// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function(testRunner) {
  TestRunner.addResult(`Check to ensure when a file is being created we don't honor events from filesystem before content is fully set.\n`);
  await TestRunner.loadModule('bindings_test_runner');

  TestRunner.addSniffer(Workspace.UISourceCode.prototype, 'checkContentUpdated', () => {
    TestRunner.addResult('ERROR! UISourceCode received checkContentUpdated but content has not been set yet when creating file. This should not happen.');
  });

  var folderLocation = 'file:///var/test';
  var fs = await (new BindingsTestRunner.TestFileSystem(folderLocation)).reportCreatedPromise();

  var fsBinding = new Persistence.FileSystemWorkspaceBinding(Persistence.isolatedFileSystemManager, Workspace.workspace);
  var fsWorkspaceBinding = Workspace.workspace.project(folderLocation);

  fs.setFileContent = setFileContentsOverride;

  TestRunner.addResult('Creating File');
  await fsWorkspaceBinding.createFile('', 'test.txt', 'file content');

  TestRunner.addResult('File Created');
  TestRunner.completeTest();

  function emulateTimeTakenToWriteContentChunk() {
    return new Promise(resolve => setTimeout(resolve, 0));
  }

  /**
   * @param {string} path
   * @param {string} content
   * @param {bool} isBase64
   * @param {function()} callback
   */
  async function setFileContentsOverride(path, content, isBase64, callback) {
    TestRunner.addResult('IsolatedFileSystem setFileContent');

    TestRunner.addResult('Emulate content dispatched to write to disk and waiting on completed event.');
    await emulateTimeTakenToWriteContentChunk();

    TestRunner.addResult('Emulate filesystem change event from filesystem.');
    InspectorFrontendHost.events.dispatchEventToListeners(
        InspectorFrontendHostAPI.Events.FileSystemFilesChangedAddedRemoved,
        {changed: [folderLocation + '/test.txt'], added: [], removed: []});

    await emulateTimeTakenToWriteContentChunk();
    TestRunner.addResult('Emulate content finished writing to disk.');
    callback();
  }

})(window.testRunner)