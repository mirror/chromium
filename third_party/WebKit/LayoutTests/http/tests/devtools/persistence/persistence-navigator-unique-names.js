// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verify that navigator view removes mapped UISourceCodes.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.loadModule('bindings_test_runner');
  Runtime.experiments.enableForTest('persistence2');

  var filesNavigator = new Sources.FilesNavigatorView();
  filesNavigator.show(UI.inspectorView.element);
  var fs1 = new BindingsTestRunner.TestFileSystem('file:///var/www/good-project/public');
  fs1.addFile('foo.js', '');
  fs1.reportCreated(function() { });

  var fs2 = new BindingsTestRunner.TestFileSystem('file:///var/www/bad-project/public');
  fs2.addFile('bar.js', '');
  fs2.reportCreated(function(){ });

  await Promise.all([
    TestRunner.waitForUISourceCode('foo.js'),
    TestRunner.waitForUISourceCode('bar.js')
  ]);

  SourcesTestRunner.dumpNavigatorView(filesNavigator, true);
  TestRunner.completeTest();
})();
