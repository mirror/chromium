function createCookieValue(settings) {
  return settings.credentials + '-' + settings.origin;
}

function createSetCookieURL(settings) {
  const params = new URLSearchParams;
  params.append('name', 'cookieName');
  params.append('value', createCookieValue(settings));

  if (settings.origin == 'same') {
    return 'resources/set-cookie.py?' + params;
  }

  if (settings.origin == 'remote') {
    return get_host_info().HTTPS_REMOTE_ORIGIN +
           '/worklets/resources/set-cookie.py?' + params;
  }

  assert_unreached('settings.origin has an invalid value.');
}

function createScriptURL(settings) {
  const params = new URLSearchParams;
  if (settings.shouldBeSent)
    params.append('value', createCookieValue(settings));
  if (settings.origin == 'same')
    return 'resources/credentials.py?' + params;
  if (settings.origin == 'remote') {
    return get_host_info().HTTPS_REMOTE_ORIGIN +
           '/worklets/resources/credentials.py?' + params;
  }
  assert_unreached('settings.origin has an invalid value.');
}

function createWorkletOptions(settings) {
  if (settings.credentials == '')
    return {};
  return { credentials: settings.credentials };
}

function runCredentialsTest(settings) {
  const worklet = get_worklet(settings.workletType);
  const setCookieURL = createSetCookieURL(settings);
  const scriptURL = createScriptURL(settings);
  const options = createWorkletOptions(settings);

  return fetch(setCookieURL, { mode: 'cors' })
      .then(() => worklet.addModule(scriptURL, options));
}

// Runs a series of tests related to credentials on a worklet.
//
// Usage:
// runCredentialsTests("paint");
function runCredentialsTests(worklet_type) {
  const worklet = get_worklet(worklet_type);
/*
  promise_test(() => {
    return runCredentialsTest({ workletType: worklet_type,
                                credentials: '',
                                origin: 'same',
                                shouldBeSent: false });
  }, 'Importing a same-origin script with the default WorkletOptions should ' +
     'omit the credentials');

  promise_test(() => {
    return runCredentialsTest({ workletType: worklet_type,
                                credentials: '',
                                origin: 'remote',
                                shouldBeSent: false });
  }, 'Importing a remote-origin script with the default WorkletOptions ' +
     'should not include the credentials');

  promise_test(() => {
    return runCredentialsTest({ workletType: worklet_type,
                                credentials: 'omit',
                                origin: 'same',
                                shouldBeSent: false });
  }, 'Importing a same-origin script with credentials=omit should omit the ' +
     'credentials');

  promise_test(() => {
    return runCredentialsTest({ workletType: worklet_type,
                                credentials: 'omit',
                                origin: 'remote',
                                shouldBeSent: false });
  }, 'Importing a remote-origin script with credentials=omit should omit the ' +
     'credentials');
*/
  promise_test(() => {
    return runCredentialsTest({ workletType: worklet_type,
                                credentials: 'same-origin',
                                origin: 'same',
                                shouldBeSent: true });
  }, 'Importing a same-origin script with credentials=same-origin should ' +
     'include the credentials');
/*
  promise_test(() => {
    return runCredentialsTest({ workletType: worklet_type,
                                credentials: 'same-origin',
                                origin: 'remote',
                                shouldBeSent: false });
  }, 'Importing a remote-origin script with credentials=same-origin should ' +
     'not include the credentials');

  promise_test(() => {
    return runCredentialsTest({ workletType: worklet_type,
                                credentials: 'include',
                                origin: 'same',
                                shouldBeSent: true });
  }, 'Importing a same-origin script with credentials=include should include ' +
     'the credentials');

  promise_test(() => {
    return runCredentialsTest({ workletType: worklet_type,
                                credentials: 'include',
                                origin: 'remote',
                                shouldBeSent: true });
  }, 'Importing a remote-origin script with credentials=include should ' +
     'include the credentials');
*/
//  promise_test(() => assert_true(false));
}
