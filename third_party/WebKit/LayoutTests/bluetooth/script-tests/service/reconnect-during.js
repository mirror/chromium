'use strict';
promise_test(() => {
  let service;
  return getHealthThermometerService()
    .then(vals => ({service} = vals))
    .then(() => service.CALLS([
      getCharacteristic('heart_rate_measurement')|
      getCharacteristics()|
      getCharacteristics('heart_rate_measurement')[UUID]
    ])
    .then(() => assert_unreached('Got descriptor unexpectedly.'), (e) => {
      assert_equals(e.name, 'NetworkError');
      assert_equals(e.message,
          'GATT Server is disconnected. Cannot retrieve characteristics. ' +
          '(Re)connect first with `device.gatt.connect`.'));
    });
}, 'disconnect() and connect() called during FUNCTION_NAME. Reject with ' +
   'NetworkError.');
