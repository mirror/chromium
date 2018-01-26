(async function(testRunner) {
  var {page, session, dp} = await testRunner.startBlank(
      `Tests tagging IndexedDB object store data`);

  var securityOrigin = 'http://127.0.0.1:8000';
  var TAGS = {
    'arraybuffer': 0,
    'date': 100,
    'array': 200
  };

  await session.evaluateAsync(`
    (function() {
      const key = 'keyfield'
      const storeName = 'test-objects-store'

      function put(store, key, text) {
        return new Promise((resolve, reject) => {
            var req = store.put({ keyfield: key, label: text });
            req.onsuccess = resolve;
            req.onerror = reject;
        });
      }

      async function addRecords() {
        var db = await new Promise(resolve => {
          var req = window.indexedDB.open('idb-fetch-data-test-db', 1);
          req.onupgradeneeded = ((event) => event.target.result.createObjectStore(storeName, { keyPath: key }));
          req.onsuccess = event => resolve(event.target.result);
        });
        return new Promise(async (resolve) => {
            var transaction = db.transaction([ storeName ], 'readwrite');
            transaction.oncomplete = resolve;
            var store = transaction.objectStore(storeName);
            await put(store, new Date(0), 'Object with date key');
            await put(store, 42, 'Object with numeric key');
            await put(store, 'Lazy fox jumped', 'Object with string key');
            await put(store, new Uint8Array([1, 2, 3, 4]), 'Object with uint array key');
            await put(store, [10, 20, 30, 40], 'Object with array key');
        });
      }

      return addRecords();
    })();
  `);

  function keyToString(key) {
    return key.value || key.description;
  }

  async function requestData(label, tags) {
    testRunner.log(label);
    var dataResponse = await dp.IndexedDB.requestData({ securityOrigin, databaseName, objectStoreName, tags, indexName: '', skipCount: 0, pageSize: 10 });
    var data = dataResponse.result.objectStoreDataEntries;
    testRunner.log(data.map(record => `${keyToString(record.primaryKey)} => ${record.tag === undefined ? '(no tag)' : record.tag}`).sort().join('\n'));
    testRunner.log('');
    var taggedKeys = [];
    var i = 0;
    for (var { primaryKey } of data) {
      if (!primaryKey.objectId)
        continue;
      taggedKeys.push({ primaryKeyRemoteID: primaryKey.objectId, tag: TAGS[primaryKey.subtype]});
    }
    return taggedKeys;
  }

  async function removeRecord(id) {
    await session.evaluateAsync(`
      (function() {
        const key = 'keyfield'
        const storeName = 'test-objects-store'

        function put(store, key, text) {
          return new Promise((resolve, reject) => {
              var req = store.put({ keyfield: key, label: text });
              req.onsuccess = resolve;
              req.onerror = reject;
          });
        }

        async function deleteRecord() {
          var db = await new Promise(resolve => {
            var req = window.indexedDB.open('idb-fetch-data-test-db', 1);
            req.onupgradeneeded = ((event) => event.target.result.createObjectStore(storeName, { keyPath: key }));
            req.onsuccess = event => resolve(event.target.result);
          });
          return new Promise((resolve, reject) => {
              var transaction = db.transaction([ storeName ], 'readwrite');
              transaction.oncomplete = resolve;
              var store = transaction.objectStore(storeName);
              var req = store.delete(${id});
              req.onsuccess = resolve;
              req.onerror = reject;
          });
        }

        return deleteRecord();
      })();
    `);
  }

  var databaseName = (await dp.IndexedDB.requestDatabaseNames({ securityOrigin })).result.databaseNames[0];
  var objectStoreName = (await dp.IndexedDB.requestDatabase({ securityOrigin, databaseName })).result.databaseWithObjectStores.objectStores[0].name;


  var keys = await requestData('New database');
  keys = await requestData('Data unchanged', keys);
  await removeRecord(42);
  keys = await requestData('Removed integer key', keys);
  await removeRecord('new Date(0)');
  keys = await requestData('Removed date key', keys);

  await dp.IndexedDB.deleteDatabase({ securityOrigin , databaseName });

  testRunner.log('Remaining databases: ' + (await dp.IndexedDB.requestDatabaseNames({ securityOrigin })).result.databaseNames.length);

  testRunner.completeTest();
})
