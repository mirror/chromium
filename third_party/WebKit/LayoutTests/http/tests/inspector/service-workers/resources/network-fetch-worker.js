self.onmessage = (event) => {
  fetch(new Request(event.data.url, event.data.init))
      .then(
        (response) => {
          return response.text();
        },
        () => {
          event.source.postMessage('FETCH_FAILED');
        })
      .then((text) => {
        event.source.postMessage(text);
      });
};
