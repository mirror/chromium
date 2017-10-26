self.addEventListener('fetch', e => {
    const obj = {reservedClientId: e.reservedClientId, url: e.request.url};
    e.respondWith(new Response(JSON.stringify(obj)));
  });

self.addEventListener('message', e => {
    url = e.data;
    self.clients.matchAll().then(clients => {
        for (let i = 0; i < clients.length; i++) {
          if (clients[i].url == url) {
            e.source.postMessage({clientId: clients[i].id});
            return;
          }
        }
        e.source.postMessage('error: client not found');
      });
  });
