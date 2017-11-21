'use strict';

// Map of id => function that releases a lock.

const held = new Map();

self.addEventListener('message', e => {
  switch (e.data.op) {
  case 'request':
    navigator.locks.acquire(
      e.data.scope, {mode: e.data.mode || 'exclusive'}, lock => {
        let release;
        const promise = new Promise(r => { release = r; });
        held.set(e.data.id, release);
        self.postMessage({ack: 'request', id: e.data.id});
        return promise;
      });
    break;

  case 'release':
    held.get(e.data.id)();
    held.delete(e.data.id);
    self.postMessage({ack: 'release', id: e.data.id});
    break;
  }
});
