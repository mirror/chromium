'use strict';

// Map of id => function that releases a flag.

const held = new Map();

self.addEventListener('message', e => {
  switch (e.data.op) {
  case 'request':
    self.requestFlag(e.data.scope, e.data.mode)
      .then(flag => {
        let release;
        const promise = new Promise(r => { release = r; });
        held.set(e.data.id, release);
        flag.waitUntil(promise);
        self.postMessage({ack: 'request', id: e.data.id});
      });
    break;

  case 'release':
    held.get(e.data.id)();
    held.delete(e.data.id);
    self.postMessage({ack: 'release', id: e.data.id});
    break;
  }
});
