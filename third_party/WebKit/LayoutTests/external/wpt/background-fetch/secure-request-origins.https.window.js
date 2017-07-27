// META: script=/service-workers/service-worker/resources/test-helpers.sub.js
// META: script=resources/utils.js
'use strict';

promise_test(t => {
  return registerAndActivateServiceWorker(t)
      .then(serviceWorkerRegistration => {
        var bgFetch = serviceWorkerRegistration.backgroundFetch;
        return Promise.all([
          bgFetch.fetch(uniqueTag(), 'https://example.com')
                 .catch(unreached_rejection(
                     t, 'https fetch should register ok')),

          bgFetch.fetch(uniqueTag(), 'http://localhost')
                 .catch(unreached_rejection(
                     t, 'localhost http fetch should register ok')),

          bgFetch.fetch(uniqueTag(), 'http://127.0.0.1')
                 .catch(unreached_rejection(
                     t, 'localhost IPv4 http fetch should register ok')),

          bgFetch.fetch(uniqueTag(), 'http://[::1]')
                 .catch(unreached_rejection(
                     t, 'localhost IPv6 http fetch should register ok')),

          promise_rejects(t, new TypeError(),
                          bgFetch.fetch(uniqueTag(), 'http://example.com'),
                          'non-localhost http fetch should reject'),

          promise_rejects(t, new TypeError(),
                          bgFetch.fetch(uniqueTag(), 'http://192.0.2.1'),
                          'non-localhost IPv4 http fetch should reject'),

          promise_rejects(t, new TypeError(),
                          bgFetch.fetch(uniqueTag(), 'http://[2001:db8::1]'),
                          'non-localhost IPv6 http fetch should reject'),

          promise_rejects(t, new TypeError(),
                          bgFetch.fetch(uniqueTag(), 'wss:localhost'),
                          'wss fetch should reject'),

          promise_rejects(t, new TypeError(),
                          bgFetch.fetch(uniqueTag(), 'file:///'),
                          'file fetch should reject'),

          promise_rejects(t, new TypeError(),
                          bgFetch.fetch(uniqueTag(), ['https://example.com',
                                                  'http://example.com']),
                          'https and non-localhost http fetch should reject'),

          promise_rejects(t, new TypeError(),
                          bgFetch.fetch(uniqueTag(), ['http://example.com',
                                                     'https://example.com']),
                          'non-localhost http and https fetch should reject'),
        ]);
      });
}, 'Only requests to https:// and http://localhost are allowed.');