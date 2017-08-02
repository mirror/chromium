// META: script=/service-workers/service-worker/resources/test-helpers.sub.js
// META: script=resources/utils.js
'use strict';

// FIXME: Split this into multiple promise_tests since Promise.all only gives
// one non-deterministic failure if things go wrong.
promise_test(t => {
  return registerAndActivateServiceWorker(t)
      .then(serviceWorkerRegistration => {
        var bgFetch = serviceWorkerRegistration.backgroundFetch;
        return Promise.all([
          bgFetch.fetch(uniqueTag(), 'https://example.com')
                 .catch(unreached_rejection(
                     t, 'https fetch should register ok')),

          // http://localhost is not tested here since the correct behavior from
          // https://w3c.github.io/webappsec-secure-contexts/#is-origin-trustworthy
          // depends on whether the UA conforms to the name resolution rules in
          // https://tools.ietf.org/html/draft-west-let-localhost-be-localhost

          bgFetch.fetch(uniqueTag(), 'http://127.0.0.1')
                 .catch(unreached_rejection(
                     t, 'loopback IPv4 http fetch should register ok')),

          bgFetch.fetch(uniqueTag(), 'http://[::1]')
                 .catch(unreached_rejection(
                     t, 'loopback IPv6 http fetch should register ok')),

          promise_rejects(t, new TypeError(),
                          bgFetch.fetch(uniqueTag(), 'http://example.com'),
                          'non-loopback http fetch should reject'),

          promise_rejects(t, new TypeError(),
                          bgFetch.fetch(uniqueTag(), 'http://192.0.2.1'),
                          'non-loopback IPv4 http fetch should reject'),

          promise_rejects(t, new TypeError(),
                          bgFetch.fetch(uniqueTag(), 'http://[2001:db8::1]'),
                          'non-loopback IPv6 http fetch should reject'),

          promise_rejects(t, new TypeError(),
                          bgFetch.fetch(uniqueTag(), 'wss:127.0.0.1'),
                          'wss fetch should reject'),

          promise_rejects(t, new TypeError(),
                          bgFetch.fetch(uniqueTag(), 'file:///'),
                          'file fetch should reject'),

          promise_rejects(t, new TypeError(),
                          bgFetch.fetch(uniqueTag(), ['https://example.com',
                                                  'http://example.com']),
                          'https and non-loopback http fetch should reject'),

          promise_rejects(t, new TypeError(),
                          bgFetch.fetch(uniqueTag(), ['http://example.com',
                                                     'https://example.com']),
                          'non-loopback http and https fetch should reject'),
        ]);
      });
}, 'Only requests to https:// and loopback http:// are allowed.');