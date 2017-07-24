'use strict';
promise_test(() => {
  let device, service;
  return getHealthThermometerService()
    .then(_ => ({device, service} = _))
    .then(() => {
      let promise = assert_promise_rejects_with_message(service.CALLS([
        getCharacteristic('heart_rate_measurement')|
        getCharacteristics()|
        getCharacteristics('heart_rate_measurement')[UUID]
      ]), new DOMException(
          'GATT Server is disconnected. Cannot retrieve characteristics. ' +
          '(Re)connect first with `device.gatt.connect`.',
          'NetworkError'));
      device.gatt.disconnect();
      return device.gatt.connect().then(() => promise);
    });
}, 'disconnect() and connect() called during FUNCTION_NAME. Reject with ' +
   'NetworkError.');
