function openWindow(url) {
  return new Promise(resolve => {
      let win = window.open(url, '_blank');
      add_completion_callback(() => win.close());
      window.onmessage = e => {
        assert_equals(e.data, 'LOADED');
        resolve(win);
      };
    });
}

// Runs a series of tests related to service worker interception for a worklet.
//
// Usage:
// runServiceWorkerInterceptionTests("paint");
function runServiceWorkerInterceptionTests(worklet_type) {
    const worklet = get_worklet(worklet_type);

    promise_test(t => {
        const kWindowURL = 'resources/addmodule-frame.html';
        const kServiceWorkerScriptURL = 'resources/service-worker.js';
        // This shouldn't contain the 'resources/' prefix because this will be
        // imported from a html file under resources/.
        const kWorkletScriptURL = 'non-existent-worklet-script.js';

        return service_worker_unregister_and_register(
            t, kServiceWorkerScriptURL, kWindowURL)
          .then(r => wait_for_state(t, r.installing, 'activated'))
          .then(() => openWindow(kWindowURL))
          .then(win => {
              const promise = new Promise(resolve => window.onmessage = resolve);
              win.postMessage({ type: worklet_type,
                                script_url: 'non-existent-worklet-script.js' }, '*');
              return promise;
            })
          .then(msg_event => assert_equals(msg_event.data, 'RESOLVED'));
    }, 'Importing a script from a controlled document should be intercepted by ' +
       'a service worker.');

    promise_test(t => {
        const kWindowURL = 'resources/addmodule-frame.html';
        const kServiceWorkerScriptURL = 'resources/service-worker.js?inscope-script';
        // This shouldn't contain the 'resources/' prefix because this will be
        // imported from a html file under resources/.
        const kWorkletScriptURL = 'non-existent-worklet-script.js';

        return service_worker_unregister_and_register(
            t, kServiceWorkerScriptURL, 'resources/' + kWorkletScriptURL)
          .then(r => wait_for_state(t, r.installing, 'activated'))
          .then(() => openWindow(kWindowURL))
          .then(win => {
              assert_equals(win.navigator.serviceWorker.controller, undefined);
              const promise = new Promise(resolve => window.onmessage = resolve);
              win.postMessage({ type: worklet_type,
                                script_url: 'non-existent-worklet-script.js' }, '*');
              return promise;
            })
          .then(msg_event => assert_equals(msg_event.data, 'RESOLVED'));
    }, 'Importing a script from a non-controlled document should not be ' +
       'intercepted by a service worker even if the script is under the ' +
       'service worker scope.');
}
