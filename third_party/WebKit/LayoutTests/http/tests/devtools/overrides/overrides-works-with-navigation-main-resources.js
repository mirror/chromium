// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Ensures when cross-origin navigating it can apply overrides to new main resource.\n`);
  await TestRunner.loadModule('bindings_test_runner');

  TestRunner.addResult('Enabling project');
  BindingsTestRunner.setOverridesEnabled(true);
  await new Promise(
    resolve => TestRunner.addSniffer(Persistence.networkPersistenceManager, '_patternsUpdatedForTest', resolve, false));

  TestRunner.addResult('Creating project for localhost:8000');
  var {project, testFileSystem} = await BindingsTestRunner.createOverrideProject('file:///tmp');
  testFileSystem.addFile('localhost%3a8000/devtools/network/resources/empty.html', '<html><head><script src="blocking.js"></script></head><body></body></html>');
  testFileSystem.addFile('localhost%3a8000/devtools/network/resources/blocking.js', '// W00T content!');

  TestRunner.addResult('Binding project to localhost:8000');
  Persistence.networkPersistenceManager.addFileSystemOverridesProject('localhost:8000', project);

  TestRunner.addResult('Navigating to localhost:8000');
  TestRunner.navigate('http://localhost:8000/devtools/network/resources/empty.html');

  TestRunner.addResult('Waiting for main resource (empty.html)');
  var mainRequest = await TestRunner.waitForEvent(SDK.NetworkManager.Events.RequestFinished, TestRunner.networkManager, request => request.name() === 'empty.html');
  var blockingRequest = await TestRunner.waitForEvent(SDK.NetworkManager.Events.RequestFinished, TestRunner.networkManager, request => request.name() === 'blocking.js');

  TestRunner.addResult('Fetching response for main resource');
  var mainContent = await mainRequest.requestContent();
  TestRunner.addResult('');
  TestRunner.addResult('Content for main resource:');
  TestRunner.addResult(mainContent);
  TestRunner.addResult('');

  var blockingContent = await blockingRequest.requestContent();
  TestRunner.addResult('Content for blocking.js:');
  TestRunner.addResult(blockingContent);
  TestRunner.addResult('');

  TestRunner.completeTest();
})();
