const cacheName = 'c8-cache-test';
const scriptName = 'v8-cache-script.js';

let continue_install;

self.addEventListener('install', (event) => {
    event.waitUntil(
      new Promise((resolve) => { continue_install = resolve; })
        .then(() => caches.open(cacheName))
        .then((cache) => cache.add(new Request(scriptName))));
  });

self.addEventListener('message', (event) => {
    continue_install();
  });

self.addEventListener('fetch', (event) => {
    if (event.request.url.indexOf(scriptName) == -1) {
      return;
    }
    event.respondWith(
      caches.open(cacheName)
        .then((cache) => {
          return cache.match(new Request(scriptName));
        })
    );
  });
