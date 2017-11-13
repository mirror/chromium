function openWindow(url) {
  return new Promise(resolve => {
      let win = window.open(url, '_blank');
      add_result_callback(() => win.close());
      window.onmessage = e => {
        assert_equals(e.data, 'LOADED');
        resolve(win);
      };
    });
}

// Runs a series of tests related to content security policy on a worklet.
//
// Usage:
// runContentSecurityPolicyTests("paint");
function runContentSecurityPolicyTests(worklet_type) {
    const worklet = get_worklet(worklet_type);

    promise_test(t => {
        const kWindowURL =
            'resources/addmodule-window.html?pipe=header(' +
            'Content-Security-Policy, script-src \'self\' \'unsafe-inline\')';
        const kScriptURL = get_host_info().HTTPS_REMOTE_ORIGIN +
                           '/worklets/resources/import-empty-worklet-script.js' +
                           '?pipe=header(Access-Control-Allow-Origin, *)';

        return openWindow(kWindowURL).then(win => {
              const promise = new Promise(r => window.onmessage = r);
              win.postMessage({ type: worklet_type,
                                script_url: kScriptURL }, '*');
              return promise;
            })
          .then(msg_event => assert_equals(msg_event.data, 'REJECTED'));
    }, 'Importing a remote-origin worklet script should be blocked by the ' +
       'script-src \'self\' directive.');

    promise_test(t => {
        const kWindowURL =
            'resources/addmodule-window.html?pipe=header(' +
            'Content-Security-Policy, script-src \'self\' \'unsafe-inline\')';
        const kScriptURL = 'import-remote-origin-empty-worklet-script.sub.js';

        return openWindow(kWindowURL).then(win => {
              const promise = new Promise(r => window.onmessage = r);
              win.postMessage({ type: worklet_type,
                                script_url: kScriptURL }, '*');
              return promise;
            })
          .then(msg_event => assert_equals(msg_event.data, 'REJECTED'));
    }, 'Importing a remote-origin script from a same-origin worklet script ' +
       'should be blocked by the script-src \'self\' directive.');

    promise_test(t => {
        const kWindowURL =
            'resources/addmodule-window.html?pipe=header(' +
            'Content-Security-Policy, script-src * \'unsafe-inline\')';
        const kScriptURL = get_host_info().HTTPS_REMOTE_ORIGIN +
                           '/worklets/resources/empty-worklet-script.js' +
                           '?pipe=header(Access-Control-Allow-Origin, *)';

        return openWindow(kWindowURL).then(win => {
              const promise = new Promise(r => window.onmessage = r);
              win.postMessage({ type: worklet_type,
                                script_url: kScriptURL }, '*');
              return promise;
            })
          .then(msg_event => assert_equals(msg_event.data, 'RESOLVED'));
    }, 'Importing a remote-origin worklet script should not be blocked ' +
       'because the script-src * directive allows it.');

    promise_test(t => {
        const kWindowURL =
            'resources/addmodule-window.html?pipe=header(' +
            'Content-Security-Policy, script-src * \'unsafe-inline\')';
        // A worklet on HTTPS_REMOTE_ORIGIN will import a child script on
        // HTTPS_REMOTE_ORIGIN.
        const kScriptURL =
            get_host_info().HTTPS_REMOTE_ORIGIN +
            '/worklets/resources/import-empty-worklet-script.js' +
            '?pipe=header(Access-Control-Allow-Origin, *)';

        return openWindow(kWindowURL).then(win => {
              const promise = new Promise(r => window.onmessage = r);
              win.postMessage({ type: worklet_type,
                                script_url: kScriptURL }, '*');
              return promise;
            })
          .then(msg_event => assert_equals(msg_event.data, 'RESOLVED'));
    }, 'Importing a remote-origin script from a remote-origin worklet script '+
       'should not be blocked because the script-src * directive allows it.');

    promise_test(t => {
        const kWindowURL =
            'resources/addmodule-window.html?pipe=header(' +
            'Content-Security-Policy, worker-src \'self\' \'unsafe-inline\')';
        const kScriptURL = get_host_info().HTTPS_REMOTE_ORIGIN +
                           '/worklets/resources/empty-worklet-script.js' +
                           '?pipe=header(Access-Control-Allow-Origin, *)';

        return openWindow(kWindowURL).then(win => {
              const promise = new Promise(r => window.onmessage = r);
              // Importing a remote-origin script should not be blocked by the
              // worker-src directive.
              win.postMessage({ type: worklet_type,
                                script_url: kScriptURL }, '*');
              return promise;
            })
          .then(msg_event => assert_equals(msg_event.data, 'RESOLVED'));
    }, 'Importing a remote-origin worklet script should not be blocked by ' +
       'the worker-src directive.');

}
