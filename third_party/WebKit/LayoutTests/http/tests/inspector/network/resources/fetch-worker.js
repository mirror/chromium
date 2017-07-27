self.onmessage = (event) => {
  fetch(new Request(event.data.url, event.data.init))
      .then(
        (response) => {
          return response.text();
        },
        () => {
          self.postMessage('FETCH_FAILED');
        })
      .then((text) => {
        self.postMessage(text);
      });
};
