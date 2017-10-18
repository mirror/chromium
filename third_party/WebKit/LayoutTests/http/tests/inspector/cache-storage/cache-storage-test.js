// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var initialize_CacheStorageTest = function() {
    InspectorTest.preloadModule("application_test_runner");
    InspectorTest.preloadPanel("resources");
}

// function onCacheStorageError(e)
// {
//     console.error("CacheStorage error: " + e);
// }

// function createCache(cacheName)
// {
//     return caches.open(cacheName).catch(onCacheStorageError);
// }

// function addCacheEntry(cacheName, requestUrl, responseText)
// {
//     return caches.open(cacheName)
//         .then(function(cache) {
//             var request = new Request(requestUrl);
//             var myBlob = new Blob();
//             var init = { "status" : 200 , "statusText" : responseText };
//             var response = new Response(myBlob, init);
//             return cache.put(request, response);
//         })
//         .catch(onCacheStorageError);
// }

// function deleteCache(cacheName)
// {
//     return caches.delete(cacheName)
//         .then(function(success) {
//             if (!success)
//                 onCacheStorageError("Could not find cache " + cacheName);
//         }).catch(onCacheStorageError);
// }

// function deleteCacheEntry(cacheName, requestUrl)
// {
//     return caches.open(cacheName)
//         .then((cache) => cache.delete(new Request(requestUrl)))
//         .catch(onCacheStorageError);
// }

// function clearAllCaches()
// {
//     return caches.keys()
//         .then((keys) => Promise.all(keys.map((key) => caches.delete(key))))
//         .catch(onCacheStorageError.bind(this, undefined));
// }
