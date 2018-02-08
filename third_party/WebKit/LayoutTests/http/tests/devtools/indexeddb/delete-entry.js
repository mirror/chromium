// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  async function deleteAndDump(label, database, objectStore, index) {
    var view = await await ApplicationTestRunner.showObjectStoreView(database, objectStore, index);
    TestRunner.addResult(label);
    view = await ApplicationTestRunner.showObjectStoreView('database1', 'objectStore1', 'index1')
    node = view._dataGrid.rootNode().children[0];
    node.select();
    await view._deleteButtonClicked(node);
    await ApplicationTestRunner.dumpObjectStores();
  }

  TestRunner.addResult(`Tests object store and index entry deletion.\n`);
  await TestRunner.loadModule('application_test_runner');
    // Note: every test that uses a storage API must manually clean-up state from previous tests.
  await ApplicationTestRunner.resetState();

  // Switch to resources panel.
  await TestRunner.showPanel('resources');
  var databaseAddedPromise = TestRunner.addSnifferPromise(Resources.IndexedDBTreeElement.prototype, '_addIndexedDB');
  await ApplicationTestRunner.createDatabaseAsync('database1');
  UI.panels.resources._sidebar.indexedDBListTreeElement.refreshIndexedDB();
  await databaseAddedPromise;
  UI.panels.resources._sidebar.indexedDBListTreeElement.expand();

  var idbDatabaseTreeElement = UI.panels.resources._sidebar.indexedDBListTreeElement._idbDatabaseTreeElements[0];
  await ApplicationTestRunner.createObjectStoreAsync('database1', 'objectStore1', 'index1');
  idbDatabaseTreeElement._refreshIndexedDB();
  await TestRunner.addSnifferPromise(Resources.IDBIndexTreeElement.prototype, '_updateTooltip');

  await ApplicationTestRunner.addIDBValueAsync('database1', 'objectStore1', 'testKey1', 'testValue');
  await ApplicationTestRunner.addIDBValueAsync('database1', 'objectStore1', 'testKey2', 'testValue');

  idbDatabaseTreeElement._refreshIndexedDB();
  await TestRunner.addSnifferPromise(Resources.IDBIndexTreeElement.prototype, '_updateTooltip');
  TestRunner.addResult('Initial state');
  ApplicationTestRunner.dumpIndexedDBTree();
  await ApplicationTestRunner.dumpObjectStores();

  await deleteAndDump('Deleting from the object store view', 'database1', 'objectStore1');
  await deleteAndDump('Deleting from the index view', 'database1', 'objectStore1', 'index1');

  TestRunner.completeTest();
})();
