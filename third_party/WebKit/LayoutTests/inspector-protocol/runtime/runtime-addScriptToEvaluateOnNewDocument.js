// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function(testRunner) {
  let {page, session, dp} = await testRunner.startBlank('');
  let identifiers = (await Promise.all([
    dp.Page.addScriptToEvaluateOnNewDocument({source: '239'}),
    dp.Page.addScriptToEvaluateOnNewDocument({source: '({a:42})'}),
  ])).map(obj => obj.result.identifier);
  testRunner.log('Navigating..');
  session.navigate('../resources/blank.html');
  let results = await Promise.all([
    dp.Page.onceEvaluatedOnNewDocument(),
    dp.Page.onceEvaluatedOnNewDocument()
  ]);
  testRunner.logMessage(results);
  await dp.Page.removeScriptToEvaluateOnNewDocument({identifier: identifiers[0]});
  testRunner.log('Navigating..');
  session.navigate('../resources/blank.html');
  testRunner.logMessage(await dp.Page.onceEvaluatedOnNewDocument());
  testRunner.completeTest();
})
