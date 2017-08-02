// Depends on /service-workers/service-worker/resources/test-helpers.sub.js
function registerAndActivateServiceWorker(t) {
  var script = 'resources/sw.js';
  var scope = 'resources/scope' + location.pathname;
  var serviceWorkerRegistration;
  return service_worker_unregister_and_register(t, script, scope)
      .then(r => {
        serviceWorkerRegistration = r;
        add_completion_callback(() => {
          serviceWorkerRegistration.unregister();
        });
        return wait_for_state(t, serviceWorkerRegistration.installing,
                              'activated');
      })
      .then(() => serviceWorkerRegistration);
}

var _nextBackgroundFetchTag = 0;
function uniqueTag() {
  return 'tag' + _nextBackgroundFetchTag++;
}