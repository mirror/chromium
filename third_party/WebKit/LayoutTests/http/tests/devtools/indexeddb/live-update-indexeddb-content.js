// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that the IndexedDB database content live updates.\n`);
  await TestRunner.loadModule('application_test_runner');
    // Note: every test that uses a storage API must manually clean-up state from previous tests.
  await ApplicationTestRunner.resetState();

  var panel = await TestRunner.showPanel('resources');

  let indexedDBModel = TestRunner.mainTarget.model(Resources.IndexedDBModel);
  indexedDBModel._throttler._timeout = 0;
  var objectStore;
  var objectStoreView;
  var indexView;

  let promise = TestRunner.addSnifferPromise(Resources.IndexedDBTreeElement.prototype, '_addIndexedDB');
  await ApplicationTestRunner.createDatabaseAsync('database1');
  await promise;
  promise = TestRunner.addSnifferPromise(Resources.IDBObjectStoreTreeElement.prototype, 'update');
  await ApplicationTestRunner.createObjectStoreAsync('database1', 'objectStore1', 'index1');
  await promise;
  ApplicationTestRunner.dumpIndexedDBTree();

  var view = await ApplicationTestRunner.showObjectStoreView('database1', 'objectStore1');
  TestRunner.addResult('\nAdd entry to objectStore1:');
  await ApplicationTestRunner.addIDBValueAsync('database1', 'objectStore1', 'testKey', 'testValue');
  await view.getPendingUpdatePromise();
  await ApplicationTestRunner.dumpObjectStores();

  // This ensures that the view updates itself.
  view = await ApplicationTestRunner.showObjectStoreView('database1', 'objectStore1');
  TestRunner.addResult('\nDelete entry from objectStore1:');
  await ApplicationTestRunner.deleteIDBValueAsync('database1', 'objectStore1', 'testKey');
  await view.getPendingUpdatePromise();
  await ApplicationTestRunner.dumpObjectStores();

  promise = TestRunner.addSnifferPromise(Resources.IndexedDBTreeElement.prototype, 'setExpandable');
  await ApplicationTestRunner.deleteDatabaseAsync('database1');
  await promise;
  TestRunner.completeTest();
})();
