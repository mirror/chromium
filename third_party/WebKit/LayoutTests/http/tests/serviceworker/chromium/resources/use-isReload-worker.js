self.addEventListener('fetch', e => {
    if (e.request.url.indexOf('doit') != -1) {
      console.log(e.isReload);
    }
  });
