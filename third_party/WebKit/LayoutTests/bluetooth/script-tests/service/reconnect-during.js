'use strict';
promise_test(() => {
  let device, service;
  return getHealthThermometerService()
    .then(vals => ({
      device,
      service,
    } = vals))
    .then(() => {
      let promise = service.CALLS([
        getCharacteristic('heart_rate_measurement')|
        getCharacteristics()|
        getCharacteristics('heart_rate_measurement')[UUID]
      ]).then(() => assert_unreached('Got characteristic unexpectedly.'),
              (e) => {
        assert_equals(e.name, 'NetworkError');
        assert_equals(e.message,
            'GATT Server is disconnected. Cannot retrieve characteristics. ' +
            '(Re)connect first with `device.gatt.connect`.');
      });
      device.gatt.disconnect();
      return device.gatt.connect().then(() => promise);
    });
}, 'disconnect() and connect() called during FUNCTION_NAME. Reject with ' +
   'NetworkError.');
