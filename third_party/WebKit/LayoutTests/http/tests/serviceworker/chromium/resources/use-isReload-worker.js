self.addEventListener('fetch', e => {
    if (e.isReload) {
      e.respondWith(new Response('this is a reload!'));
    } else {
      e.respondWith(new Response('this is not a reload!'));
    }
  });
