// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that cache data is correctly populated in the Inspector.\n`);
  await TestRunner.loadModule('application_test_runner');
    // Note: every test that uses a storage API must manually clean-up state from previous tests.
  await ApplicationTestRunner.resetState();

  await TestRunner.showPanel('resources');

  var cacheStorageModel = TestRunner.mainTarget.model(SDK.ServiceWorkerCacheModel);
  cacheStorageModel.enable();

  function errorAndExit(error) {
    if (error)
      TestRunner.addResult(error);
    TestRunner.completeTest();
  }

  function main() {
    ApplicationTestRunner.clearAllCaches()
        .then(console.log('1'))
        .then(ApplicationTestRunner.dumpCacheTree)
        .then(console.log('1'))
        .then(ApplicationTestRunner.createCache.bind(this, 'testCache1'))
        .then(console.log('1'))
        .then(ApplicationTestRunner.createCache.bind(this, 'testCache2'))
        .then(console.log('1'))
        .then(ApplicationTestRunner.dumpCacheTree)
        .then(console.log('1'))
        .then(ApplicationTestRunner.addCacheEntry.bind(this, 'testCache1', 'http://fake.request.com/1', 'OK'))
        .then(console.log('1'))
        .then(ApplicationTestRunner.addCacheEntry.bind(this, 'testCache1', 'http://fake.request.com/2', 'Not Found'))
        .then(console.log('1'))
        .then(ApplicationTestRunner.addCacheEntry.bind(this, 'testCache2', 'http://fake.request2.com/1', 'OK'))
        .then(console.log('1'))
        .then(ApplicationTestRunner.addCacheEntry.bind(this, 'testCache2', 'http://fake.request2.com/2', 'Not Found'))
        .then(console.log('1'))
        .then(ApplicationTestRunner.dumpCacheTree)
        .then(console.log('1'))
        .then(ApplicationTestRunner.clearAllCaches)
        .then(console.log('1'))
        .then(TestRunner.completeTest)
        .then(console.log('1'))
        .catch(errorAndExit);
  }

  ApplicationTestRunner.waitForCacheRefresh(main);
})();
