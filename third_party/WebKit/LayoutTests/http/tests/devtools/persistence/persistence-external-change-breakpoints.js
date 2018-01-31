// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verify that breakpoints survive external editing.\n`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.loadModule('bindings_test_runner');
  await TestRunner.addScriptTag('resources/foo.js');
  await TestRunner.showPanel('sources');

  var testMapping = BindingsTestRunner.initializeTestMapping();
  var fs = new BindingsTestRunner.TestFileSystem('file:///var/www');
  var fsEntry = BindingsTestRunner.addFooJSFile(fs);

  TestRunner.runTestSuite([
    function addFileSystem(next) {
      fs.reportCreated(next);
    },

    async function setBreakpointInFileSystemUISourceCode(next) {
      testMapping.addBinding('foo.js');
      await BindingsTestRunner.waitForBinding('foo.js');
      var uiSourceCode = await TestRunner.waitForUISourceCode('foo.js', Workspace.projectTypes.FileSystem);
      var sourceFrame = await SourcesTestRunner.showUISourceCodePromise(uiSourceCode);
      SourcesTestRunner.setBreakpoint(sourceFrame, 2, '', true);
      await SourcesTestRunner.waitBreakpointSidebarPane();
      dumpBreakpointSidebarPane();
      next();
    },

    async function addCommitToFilesystemUISourceCode(next) {
      var uiSourceCode = await TestRunner.waitForUISourceCode('foo.js', Workspace.projectTypes.FileSystem);
      await Promise.all([
        TestRunner.addSnifferPromise(Sources.JavaScriptSourceFrame.prototype, '_restoreBreakpointsAfterEditing'),
        uiSourceCode.addRevision(`
      var w = 'some content';
      var x = 'new content';
      var y = 'more new content';`)]);
      await SourcesTestRunner.waitBreakpointSidebarPane();
      dumpBreakpointSidebarPane();
      next();
    }
  ]);

  function dumpBreakpointSidebarPane() {
    var paneElement = self.runtime.sharedInstance(Sources.JavaScriptBreakpointsSidebarPane).contentElement;
    var empty = paneElement.querySelector('.gray-info-message');
    if (empty)
      return TestRunner.textContentWithLineBreaks(empty);
    var entries = Array.from(paneElement.querySelectorAll('.breakpoint-entry'));
    for (var entry of entries) {
      var uiLocation = entry[Sources.JavaScriptBreakpointsSidebarPane._locationSymbol];
      TestRunner.addResult('    ' + uiLocation.uiSourceCode.url() + ':' + uiLocation.lineNumber);
    }
  }
})();
