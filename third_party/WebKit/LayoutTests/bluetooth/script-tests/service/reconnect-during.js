'use strict';
promise_test(() => {
  let device, fake_peripheral, service;
  return getHealthThermometerService()
    .then(_ => ({device, fake_peripheral, service} = _))
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
      return fake_peripheral.setNextGATTConnectionResponse({code: HCI_SUCCESS})
          .then(() => device.gatt.connect())
          .then(() => promise);
    });
}, 'disconnect() and connect() called during FUNCTION_NAME. Reject with ' +
   'NetworkError.');
