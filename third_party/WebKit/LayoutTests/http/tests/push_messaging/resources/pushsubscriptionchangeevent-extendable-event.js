importScripts('../../serviceworker/resources/worker-testharness.js');

test(function() {
    assert_true('PushSubscriptionChangeEvent' in self);

    var event = new PushSubscriptionChangeEvent('PushSubscriptionChangeEvent');
    assert_equals(event.type, 'PushSubscriptionChangeEvent');
    assert_idl_attribute(event, 'newSubscription');
    assert_idl_attribute(event, 'oldSubscription');
    assert_equals(event.newSubscription, null);
    assert_equals(event.oldSubscription, null);
    assert_equals(event.cancelable, false);
    assert_equals(event.bubbles, false);
    assert_inherits(event, 'waitUntil');

}, 'PushSubscriptionChangeEvent is exposed and extends ExtendableEvent.');
