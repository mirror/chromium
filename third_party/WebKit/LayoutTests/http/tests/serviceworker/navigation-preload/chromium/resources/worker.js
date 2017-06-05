self.addEventListener('activate', e => {
    e.waitUntil(self.registration.navigationPreload.enable());
  });

let resolve;
self.addEventListener('fetch', e => {
  e.respondWith(e.preloadResponse);
  });
