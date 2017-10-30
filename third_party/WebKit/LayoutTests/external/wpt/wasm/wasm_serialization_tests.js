// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function TestInstantiateInWorker() {
  return createWasmModule()
    .then((mod) => {
      var worker = new Worker("wasm_serialization_worker.js");
      return new Promise((resolve, reject) => {
        worker.onmessage = function(event) {
          resolve(event.data);
        };
        worker.postMessage(mod);
      });
    })
    .then(data => assert_equals(data, 43))
    .catch(error => assert_unreached(error));
}

// make sure it matches the name used by wasm_idb_worker.js

var db_name = "wasm_worker_idb_test";

function SaveToIDBAndLoadInWorker() {
  return createAndSaveToIndexedDB(db_name)
  .then(() => {
    var worker = new Worker("wasm_idb_worker.js");
    return new Promise((resolve, reject) => {
      worker.onmessage = function (event) {
        if (typeof (event.data) == "string") {
          resolve(event.data);
        }
      };
      worker.postMessage("load");
    })
  })
.then(data => assert_equals(data, "ok"),
    error => assert_unreached(error));
}

function SaveToIDBInWorkerAndLoadInMain() {
  var worker = new Worker("wasm_idb_worker.js");
  return new Promise((resolve, reject) => {
    worker.onmessage = function (event) {
      if (typeof (event.data) == "string") {
        resolve(event.data);
      }
    };
    worker.postMessage("save");
  }
 )
  .then(data => assert_equals(data, "ok"),
        error => assert_unreached(error))
  .then(() => loadFromIndexedDB(Promise.resolve(), db_name))
  .then(res => assert_equals(res, 2),
        assert_unreached);
}
