'use strict';
promise_test(() => {
  let service, characteristic, fake_service, fake_characteristic;
  return getMeasurementIntervalCharacteristic()
    .then(vals => ({
      service,
      characteristic,
      fake_service,
      fake_characteristic,
    } = vals))
    .then(() => characteristic.getDescriptor(user_description.name))
    .then(() => null, (e) => assert_unreached('Caught error unexpectedly.', e))
    .then(() => fake_service.removeFakeCharacteristic({
      characteristic: fake_characteristic,
    }))
    .then(() => characteristic.CALLS([
      getDescriptor(user_description.name)|
      getDescriptors(user_description.name)[UUID]|
      getDescriptors()|
      readValue()|
      writeValue(new Uint8Array(1))|
      startNotifications()]))
    .then(() => assert_unreached('Got descriptor unexpectedly.'), (e) => {
      assert_equals(e.name, 'InvalidStateError');
      assert_equals(e.message, 'GATT Characteristic no longer exists.');
    });
}, 'Characteristic gets removed. Reject with InvalidStateError.');
