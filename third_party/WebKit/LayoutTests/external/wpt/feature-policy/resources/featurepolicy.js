// Tests whether a feature that is enabled/disabled by feature policy works
// as expected.
// Arguments:
//    feature_decription: a short string describing what feature is being
//        tested. Examples: "usb.GetDevices()", "PaymentRequest()".
//    test: test created by testharness. Examples: async_test, promise_test.
//    src: URL where a feature's availability is checked. Examples:
//        "/feature-policy/resources/feature-policy-payment.html",
//        "/feature-policy/resources/feature-policy-usb.html".
//    expect_feature_available: a callback(data, feature_description) to
//        verify if a feature is avaiable or unavailable as epected.
//        Example: expect_feature_available_default(data, feature_description).
//    feature_name: Optional argument, only provided when testing iframe allow
//      attribute. "feature_name" is the feature name of a policy controlled
//      feature (https://wicg.github.io/feature-policy/#features).
//      See examples at:
//      https://github.com/WICG/feature-policy/blob/gh-pages/features.md
//    allow_attribute: Optional argument, only used for testing fullscreen or
//      payment: either "allowfullscreen" or "allowpaymentrequest" is passed.
function test_feature_availability(
    feature_decription, test, src, expect_feature_available, feature_name,
    allow_attribute) {
  let frame = document.createElement('iframe');
  frame.src = src;

  if (typeof feature_name !== 'undefined') {
    frame.allow.add(feature_name);
  }

  if (typeof allow_attribute !== 'undefined') {
    frame.setAttribute(allow_attribute, true);
  }

  window.addEventListener('message', test.step_func(evt => {
    if (evt.source === frame.contentWindow) {
      expect_feature_available(evt.data, feature_decription);
      document.body.removeChild(frame);
      test.done();
    }
  }));

  document.body.appendChild(frame);
}

// Default helper functions to test a feature's availability:
function expect_feature_available_default(data, feature_decription) {
  assert_true(data.enabled, feature_decription);
}

function expect_feature_unavailable_default(data, feature_decription) {
  assert_false(data.enabled, feature_decription);
}
