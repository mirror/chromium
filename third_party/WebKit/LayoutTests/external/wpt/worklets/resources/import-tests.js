// Runs a series of tests related to importing scripts on a worklet.
//
// Usage:
// runImportTests(workletType);
function runImportTests(worklet) {
    promise_test(() => {
        const kScriptURL = 'resources/empty-worklet-script.js';
        return worklet.addModule(kScriptURL).then(undefined_arg => {
            assert_equals(undefined_arg, undefined,
                          'Promise should resolve with no arguments.');
        });
    }, 'Importing a script resolves the given promise.');

    promise_test(() => {
        const kScriptURL = 'resources/empty-worklet-script.js';
        return Promise.all([
            worklet.addModule(kScriptURL + '?1'),
            worklet.addModule(kScriptURL + '?2'),
            worklet.addModule(kScriptURL + '?3')
        ]).then(undefined_args => {
            assert_array_equals(undefined_args,
                                [undefined, undefined, undefined],
                                'Promise should resolve with no arguments.');
        });
    }, 'Importing scripts resolves all the given promises.');

    promise_test(() => {
        const kScriptURL = 'resources/import-nested-worklet-script.js';
        return worklet.addModule(kScriptURL).then(undefined_arg => {
            assert_equals(undefined_arg, undefined,
                          'Promise should resolve with no arguments.');
        });
    }, 'Importing nested scripts resolves the given promise');

    promise_test(() => {
        const kScriptURL = 'resources/import-cyclic-worklet-script.js';
        return worklet.addModule(kScriptURL).then(undefined_arg => {
            assert_equals(undefined_arg, undefined,
                          'Promise should resolve with no arguments.');
        });
    }, 'Importing cyclic scripts resolves the given promise');

    promise_test(() => {
        const kScriptURL = 'resources/throwing-worklet-script.js';
        return worklet.addModule(kScriptURL).then(undefined_arg => {
            assert_equals(undefined_arg, undefined,
                          'Promise should resolve with no arguments.');
        });
    }, 'Importing a script which throws should still resolve the given ' +
       'promise.');

    promise_test(t => {
        const kScriptURL = 'non-existent-worklet-script.js';
        return promise_rejects(t, new DOMException('', 'AbortError'),
                               worklet.addModule(kScriptURL));
    }, 'Importing a non-existent script rejects the given promise with an ' +
       'AbortError.');

    promise_test(t => {
        const kInvalidScriptURL = 'http://invalid:123$'
        return promise_rejects(t, new DOMException('', 'SyntaxError'),
                               worklet.addModule(kInvalidScriptURL));
    }, 'Importing an invalid URL should reject the given promise with a ' +
       'SyntaxError.');

    promise_test(() => {
        const kBlob = new Blob([""], {type: 'text/javascript'});
        const kBlobURL = URL.createObjectURL(kBlob);
        return worklet.addModule(kBlobURL).then(undefined_arg => {
            assert_equals(undefined_arg, undefined);
        });
    }, 'Importing a blob URL should resolve the given promise.');

    promise_test(t => {
        const kFileURL = 'file:///empty-worklet-script.js';
        return promise_rejects(t, new DOMException('', 'AbortError'),
                               worklet.addModule(kFileURL));
    }, 'Importing a file:// URL should reject the given promise.');

    promise_test(() => {
        const kDataURL = 'data:text/javascript, // Do nothing.';
        return worklet.addModule(kDataURL).then(undefined_arg => {
            assert_equals(undefined_arg, undefined);
        });
    }, 'Importing a data URL should resolve the given promise.');

    promise_test(t => {
        const kScriptURL = 'about:blank';
        return promise_rejects(t, new DOMException('', 'AbortError'),
                               worklet.addModule(kScriptURL));
    }, 'Importing about:blank should reject the given promise.');

    promise_test(() => {
        // Specify the Access-Control-Allow-Origin header to enable cross origin
        // access.
        const kScriptURL = get_host_info().HTTP_REMOTE_ORIGIN +
                           '/worklets/resources/empty-worklet-script.js' +
                           '?pipe=header(Access-Control-Allow-Origin, *)';
        return worklet.addModule(kScriptURL).then(undefined_arg => {
            assert_equals(undefined_arg, undefined);
        });
    }, 'Importing a cross origin resource with the ' +
       'Access-Control-Allow-Origin header should resolve the given promise');

    promise_test(t => {
        // Don't specify the Access-Control-Allow-Origin header. addModule()
        // should be rejected because of disallowed cross origin access.
        const kScriptURL = get_host_info().HTTP_REMOTE_ORIGIN +
                           '/worklets/resources/empty-worklet-script.js';
        return promise_rejects(t, new DOMException('', 'AbortError'),
                               worklet.addModule(kScriptURL));
    }, 'Importing a cross origin resource without the ' +
       'Access-Control-Allow-Origin header should reject the given promise');

    promise_test(() => {
        document.cookie = "cookieName=default";
        const kScriptURL = 'resources/credentials.py';
        return worklet.addModule(kScriptURL).then(undefined_arg => {
            assert_equals(undefined_arg, undefined);
        });
    }, 'Importing a script with the default WorkletOptions should omit the ' +
       'credentials');

    promise_test(() => {
        document.cookie = "cookieName=omit";
        const kScriptURL = 'resources/credentials.py?mode=omit';
        const kOptions = { credentials: 'omit' };
        return worklet.addModule(kScriptURL, kOptions).then(undefined_arg => {
            assert_equals(undefined_arg, undefined);
        });
    }, 'Importing a script with credentials=omit should omit the credentials');

    promise_test(() => {
        document.cookie = "cookieName=include";
        const kScriptURL = 'resources/credentials.py?mode=include';
        const kOptions = { credentials: 'include' };
        return worklet.addModule(kScriptURL, kOptions).then(undefined_arg => {
            assert_equals(undefined_arg, undefined);
        });
    }, 'Importing a script with credentials=include should include the ' +
       ' credentials');

    promise_test(() => {
        const kSetCookieURL =
            get_host_info().HTTP_REMOTE_ORIGIN +
            "/worklets/resources/set-cookie.py?name=cookieName";
        const kScriptURL = get_host_info().HTTP_REMOTE_ORIGIN +
                           '/worklets/resources/credentials.py?mode=include';

        // Set a cookie in the remote origin and then start a worklet.
        return fetch(kSetCookieURL, { mode: 'no-cors', credentials: 'include'})
          .then(() => worklet.addModule(kScriptURL, { credentials: 'include' }))
          .then(undefined_arg => assert_equals(undefined_arg, undefined));
    }, 'Importing a cross origin script with credentials=include should ' +
       'include the credentials');

    // TODO(nhiroki): Add tests for "credentials=same-origin"
}
