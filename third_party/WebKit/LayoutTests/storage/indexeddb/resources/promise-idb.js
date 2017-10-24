// Quick and dirty promise wrapper of IDB.

var pdb = {
  _transformRequestToPromise: function(thisobj, func, argArray) {
    return new Promise(function(resolve, reject) {
      var request = func.apply(thisobj, argArray);
      request.onsuccess = function() {
        resolve(request.result);
      };
      request.onerror = function() {
        reject(request.error);
      };
    });
  },

  openCursor: function(txn, source, query, callback) {
    return new Promise(function(resolve, reject) {
      var request = source.openCursor(query);
      request.onerror = function() {
        reject(request.error);
      };
      request.onsuccess = function() {
        var cursor = request.result;
        var cont = false;
        var control = {
          continue: function() {cont = true;}
        };
        if (cursor) {
          callback(control, cursor.value);
          if (cont) {
            cursor.continue();
          } else {
            resolve(txn);
          }
        } else {
          resolve(txn);
        }
      };
    });
  },

  get: function(source, query) {
    return this._transformRequestToPromise(source, source.get, [query]);
  },

  getKey: function(source, query) {
    return this._transformRequestToPromise(source, source.getKey, [query]);
  },

  count: function(source, query) {
    return this._transformRequestToPromise(source, source.count, [query]);
  },

  put: function(objectStore, value, key) {
    return this._transformRequestToPromise(objectStore, objectStore.put, [value, key]);
  },

  add: function(objectStore, value, key) {
    return this._transformRequestToPromise(objectStore, objectStore.add, [value, key]);
  },

  delete: function(objectStore, query) {
    return this._transformRequestToPromise(objectStore, objectStore.delete, [query]);
  },

  clear: function(objectStore) {
    return this._transformRequestToPromise(objectStore, objectStore.clear, []);
  },

  waitForTransaction: function(txn) {
    return new Promise(function(resolve, reject) {
      txn.oncomplete = resolve;
      txn.onabort = function() {
        reject(txn.error);
      };
    });
  }
};
