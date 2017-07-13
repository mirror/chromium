'use strict';
promise_test(() => {
  let characteristic, fake_characteristic;
  return getMeasurementIntervalCharacteristic()
    .then(_ => ({characteristic, fake_characteristic} = _))
    .then(() => characteristic.getDescriptor(user_description.name))
    .then(() => null, (e) => assert_unreached('Caught error unexpectedly.', e))
    .then(() => fake_characteristic.remove())
    .then(success => assert_true(success, 'Failed to remove characteristic.'))
    .then(() => assert_promise_rejects_with_message(characteristic.CALLS([
      getDescriptor(user_description.name)|
      getDescriptors(user_description.name)[UUID]|
      getDescriptors()|
      readValue()|
      writeValue(new Uint8Array(1))|
      startNotifications()
    ]), new DOMException('GATT Characteristic no longer exists.',
                         'InvalidStateError')));
}, 'Characteristic gets removed. Reject with InvalidStateError.');
