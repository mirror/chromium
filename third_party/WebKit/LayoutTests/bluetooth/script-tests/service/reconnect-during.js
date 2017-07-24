'use strict';
promise_test(() => {
  let device, fake_peripheral, service, fake_service;
  return getHealthThermometerDeviceWithServicesDiscovered({
      filters: [{services: [health_thermometer.name]}]
  })
    .then(_ => ({device, fake_peripheral} = _))
    .then(() => fake_peripheral.addFakeService({uuid: health_thermometer.name}))
    .then(s => (fake_service = s))
    .then(() => device.gatt.getPrimaryService('health_thermometer'))
    .then(s => service = s)
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
