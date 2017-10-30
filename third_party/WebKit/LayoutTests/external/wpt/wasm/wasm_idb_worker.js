// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

importScripts('/resources/testharness.js');
importScripts('resources/load_wasm.js');
importScripts('wasm_indexeddb_test.js');

var db_name = "wasm_worker_idb_test";

onmessage = function(e) {
  if (e.data === "load") {
    loadFromIndexedDB(Promise.resolve(), db_name)
      .then(res => {
        if (res === 2) postMessage("ok");
        else postMessage("error");
      },
            error => postMessage(error));
  } else if (e.data === "save") {
    createAndSaveToIndexedDB(db_name)
      .then((m) => {
        postMessage("ok");
      },
            () => postMessage("error"));
  } else {
    postMessage("unknown message: " + e.data);
  }
}
