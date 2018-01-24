// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Editing inline styles should play nice with inline scripts.\n`);
  await TestRunner.loadModule('bindings_test_runner');

  await BindingsTestRunner.attachFrame('frame', './resources/inline-style.html', '_test_attachFrame.js');
  var uiSourceCode = Workspace.workspace.uiSourceCodes().find(uiSourceCode => {
    var url = uiSourceCode.url();
    return url.endsWith('inline-style.html') && !url.startsWith('debugger://');
  });
  await uiSourceCode.requestContent(); // prefetch content to fix flakiness
  var target = Bindings.NetworkProject.targetForUISourceCode(uiSourceCode);
  var cssModel = target.model(SDK.CSSModel);
  var debuggerModel = target.model(SDK.DebuggerModel);
  var styleSheets = cssModel.styleSheetIdsForURL(uiSourceCode.url());
  var scripts = debuggerModel.scriptsForSourceURL(uiSourceCode.url());
  var locationPool = new Bindings.LiveLocationPool();
  var i = 0;
  for (var script of scripts) {
    var rawLocation = debuggerModel.createRawLocation(script, script.lineOffset, script.columnOffset);
    Bindings.debuggerWorkspaceBinding.createLiveLocation(
      rawLocation, updateDelegate.bind(null, 'script' + i), locationPool);
    i++;
  }

  i = 0;
  for (var styleSheetId of styleSheets) {
    var header = cssModel.styleSheetHeaderForId(styleSheetId);
    var rawLocation = new SDK.CSSLocation(header, header.startLine, header.startColumn);
    Bindings.cssWorkspaceBinding.createLiveLocation(
      rawLocation, updateDelegate.bind(null, 'style' + i), locationPool);
    i++;
  }

  i = 0;
  for (var styleSheetId of styleSheets) {
    TestRunner.addResult('Adding rule' + i)
    await cssModel.addRule(styleSheetId, `.new-rule {
  --new: true;
}`, TextUtils.TextRange.createFromLocation(0, 0));
    await Promise.resolve();
    i++;
  }


  function updateDelegate(name, location) {
    var uiLocation = location.uiLocation();
    TestRunner.addResult(`LiveLocation '${name}' was updated ${uiLocation.lineNumber}:${uiLocation.columnNumber}`);
  }

  TestRunner.completeTest();
})();
