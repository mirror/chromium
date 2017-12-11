importScripts("/resources/testharness.js");

test(function() {
  assert_true(isSecureContext);
  assert_false('PaymentRequest' in self);
  assert_false('PaymentRequestUpdateEvent' in self);
}, "PaymentRequest constructor must not be exposed in worker global scope");

done();
