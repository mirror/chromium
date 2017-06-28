// A service worker that calls FetchEvent.isReload for UseCounter purposes.
self.addEventListener('fetch', e => {
    if (e.request.url.indexOf('call-isReload') != -1) {
      e.isReload;
    }
  });
