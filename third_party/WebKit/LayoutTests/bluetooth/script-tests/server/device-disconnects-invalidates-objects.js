'use strict';
promise_test(() => {
  return getHealthThermometerDevice()
    .then(({device, fake_peripheral}) => {
      let services;
      return device.gatt.CALLS([
          getPrimaryService('health_thermometer')|
          getPrimaryServices()|
          getPrimaryServices('health_thermometer')[UUID]])
        .then(s => {
          // Convert to array if necessary.
          services = [].concat(s);
          return disconnectFakeAndWaitForEvent(device, fake_peripheral);
        })
        .then(() => fake_peripheral.setNextGATTConnectionResponse({
          code: HCI_SUCCESS}))
        .then(() => device.gatt.connect())
        .then(() => services);
    })
    .then(services => {
      let promises = Promise.resolve();
      for (let service of services) {
        let error = new DOMException(
          'Service with UUID ' + service.uuid +
          ' is no longer valid. Remember to retrieve the service ' +
          'again after reconnecting.',
          'InvalidStateError');
        promises = promises.then(() =>
          assert_promise_rejects_with_message(
            service.getCharacteristic('measurement_interval'),
            error));
        promises = promises.then(() =>
          assert_promise_rejects_with_message(
            service.getCharacteristics(),
            error));
        promises = promises.then(() =>
          assert_promise_rejects_with_message(
            service.getCharacteristics('measurement_interval'),
            error));
      }
      return promises;
    });
}, 'Calls on services after device disconnects and we reconnect. ' +
   'Should reject with InvalidStateError.');
