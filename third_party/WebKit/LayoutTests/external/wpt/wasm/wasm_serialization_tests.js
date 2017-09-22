// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function TestInstantiateInWorker() {
  return createWasmModule()
    .then((mod) => {
      var worker = new Worker("wasm_serialization_worker.js");
      return new Promise((resolve, reject) => {
        worker.postMessage(mod);
        worker.onmessage = function(event) {
          resolve(event.data);
        }
      });
    })
    .then(data => assert_equals(data, 43))
    .catch(error => assert_unreached(error));
}

function SaveToIDBAndLoadInWorker() {
  return createAndSaveToIndexedDB()
  .then(() => {
    var worker = new Worker("wasm_idb_worker.js");
    return new Promise((resolve, reject) => {
      worker.postMessage("load");
      worker.onmessage = function (event) {
        if (typeof (event.data) == "string") {
          resolve(event.data);
        }
      }
    })
  })
.then(data => assert_equals(data, "ok"),
    error => assert_unreached(error));
}

function SaveToIDBInWorkerAndLoadInMain() {
  var worker = new Worker("wasm_idb_worker.js");
  return new Promise((resolve, reject) => {
    worker.postMessage("save");
    worker.onmessage = function (event) {
      if (typeof (event.data) == "string") {
        resolve(event.data);
      }
    }
  }
 )
  .then(data => assert_equals(data, "ok"),
    error => assert_unreached(error))
  .then(() => loadFromIndexedDB(Promise.resolve()))
  .then(res => assert_equals(res, 2),
    assert_unreached);
}
