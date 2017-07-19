function grant_permission(feature_name, origin) {
  if (window.testRunner) {
    window.testRunner.setPermission(
        feature_name, 'granted', origin, location.origin);
  }
}

function assert_available_in_iframe(
    feature_name, test, location, expected, allow_attributes) {
  let frame = document.createElement('iframe');
  if (allow_attributes)
    frame.allow = allow_attributes;
  frame.src = location;

  grant_permission(feature_name, frame.src);

  window.addEventListener('message', test.step_func(evt => {
    if (evt.source == frame.contentWindow) {
      assert_equals(evt.data, expected);
      document.body.removeChild(frame);
      test.done();
    }
  }));

  document.body.appendChild(frame);
}

function run_permission_default_header_policy_tests(
    cross_origin_url, feature_name, error_name, feature_promise_factory) {
  // This may be the version of the page loaded up in an iframe. If so, just
  // post the result of running the feature promise back to the parent.
  if (location.hash == '#iframe') {
    Promise.resolve()
        .then(() => feature_promise_factory())
        .then(
            () => {
              window.parent.postMessage('#OK', '*');
            },
            error => {
              window.parent.postMessage('#' + error.name, '*');
            });
    return;
  }

  grant_permission(feature_name, location.origin);

  // Run the various tests.
  // 1. Top level frame.
  promise_test(
      () => feature_promise_factory(),
      'Default "' + feature_name +
          '" feature policy ["self"] allows the top-level document.');

  // 2. Same-origin iframe.
  // Append #iframe to the URL so we can detect the iframe'd version of the
  // page.
  var same_origin_frame = location.pathname + '#iframe';
  async_test(
      t => {
        assert_available_in_iframe(feature_name, t, same_origin_frame, '#OK');
      },
      'Default "' + feature_name +
          '" feature policy ["self"] allows same-origin iframes.');

  // 3. Cross-origin iframe.
  var cross_origin_frame = cross_origin_url + same_origin_frame;
  async_test(
      t => {
        assert_available_in_iframe(
            feature_name, t, cross_origin_frame, '#' + error_name);
      },
      'Default "' + feature_name +
          '" feature policy ["self"] disallows cross-origin iframes.');

  // 4. Cross-origin iframe with "allow" attribute.
  async_test(
      t => {
        assert_available_in_iframe(
            feature_name, t, cross_origin_frame, '#OK', feature_name);
      },
      'Feature policy "' + feature_name +
          '" can be enabled in cross-origin iframes using "allowed" attribute.');
}