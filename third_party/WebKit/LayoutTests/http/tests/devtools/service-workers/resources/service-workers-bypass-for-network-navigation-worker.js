self.addEventListener('fetch', function(event) {
  event.respondWith(new Response(
      '<body>From the service worker</body>',
      {
        headers: [['content-type', 'text/html']]
      }));
});
