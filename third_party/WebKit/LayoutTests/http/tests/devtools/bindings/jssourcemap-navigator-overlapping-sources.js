// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Verify that JavaScript SourceMap handle different sourcemaps with overlapping sources.`);
  await TestRunner.loadModule('sources_test_runner');
  await TestRunner.loadModule('bindings_test_runner');

  var sourcesNavigator = new Sources.SourcesNavigatorView();
  sourcesNavigator.show(UI.inspectorView.element);

  TestRunner.markStep('dumpInitialNavigator');
  SourcesTestRunner.dumpNavigatorView(sourcesNavigator, false);

  TestRunner.markStep('attachFrame1');
  await Promise.all([
    BindingsTestRunner.attachFrame('frame1', './resources/jssourcemaps-with-overlapping-sources/frame1.html', '_test_create-iframe1.js'),
    BindingsTestRunner.waitForSourceMap('script1.js.map'),
  ]);
  SourcesTestRunner.dumpNavigatorView(sourcesNavigator, false);

  TestRunner.markStep('attachFrame2');
  await Promise.all([
    BindingsTestRunner.attachFrame('frame2', './resources/jssourcemaps-with-overlapping-sources/frame1.html', '_test_create-iframe2.js'),
    BindingsTestRunner.waitForSourceMap('script1.js.map'),
  ]);
  SourcesTestRunner.dumpNavigatorView(sourcesNavigator, false);

  TestRunner.markStep('attachFrame3');
  await Promise.all([
    BindingsTestRunner.attachFrame('frame3', './resources/jssourcemaps-with-overlapping-sources/frame2.html', '_test_create-iframe3.js'),
    BindingsTestRunner.waitForSourceMap('script2.js.map')
  ]);
  SourcesTestRunner.dumpNavigatorView(sourcesNavigator, false);

  TestRunner.markStep('detachFrame2');
  await BindingsTestRunner.detachFrame('frame2', '_test_detachFrame2.js');
  SourcesTestRunner.dumpNavigatorView(sourcesNavigator, false);

  TestRunner.markStep('detachFrame3');
  await BindingsTestRunner.detachFrame('frame3', '_test_detachFrame3.js');
  SourcesTestRunner.dumpNavigatorView(sourcesNavigator, false);

  TestRunner.markStep('detachFrame1');
  await BindingsTestRunner.detachFrame('frame1', '_test_detachFrame1.js');
  SourcesTestRunner.dumpNavigatorView(sourcesNavigator, false);

  TestRunner.completeTest();
})();
