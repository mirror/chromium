// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

self.addEventListener('activate', function(event) {
  event.waitUntil(self.clients.claim()); // Become available to all pages
});

function createHtmlNoSniffResponse() {
  var headers = new Headers();
  headers.append('Content-Type', 'text/html');
  headers.append('X-Content-Type-Options', 'nosniff');
  return new Response("Response created by service worker",
                      { status: 200, headers: headers });
}

var previousResponse = undefined;
var previousUrl = undefined;

self.addEventListener('fetch', function(event) {
  if (event.request.url.endsWith("data_from_service_worker")) {
    event.respondWith(createHtmlNoSniffResponse());
  } else {
    if (previousUrl == event.request.url && previousResponse) {
      event.respondWith(previousResponse.clone());
    } else {
      event.respondWith(new Promise(function(resolve, reject) {
          fetch(event.request)
              .then(function(response) {
                  previousResponse = response;
                  previousUrl = event.request.url;

                  reject("Expected error from service worker");
              })
              .catch(function(error) {
                  reject("Unexpected error: " + error);
              });
      }));
    }
  }
});
