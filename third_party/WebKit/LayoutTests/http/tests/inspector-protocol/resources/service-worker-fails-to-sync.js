async function postMessageAndFail() {
  for (var client of await self.clients.matchAll())
    client.postMessage('Sync received');
  throw new Error('Sync will not be completed');
}

self.addEventListener('activate', event => event.waitUntil(clients.claim()));
self.addEventListener('sync', event => event.waitUntil(postMessageAndFail()));
