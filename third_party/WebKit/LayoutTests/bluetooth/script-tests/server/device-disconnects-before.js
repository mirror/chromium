'use strict';
promise_test(t => {
  return getConnectedHealthThermometerDevice()
    .then(({device, fake_peripheral}) => {
      return disconnectFakeAndWaitForEvent(device, fake_peripheral)
        .then(() => assert_promise_rejects_with_message(
          device.gatt.CALLS([
            getPrimaryService('heart_rate')|
            getPrimaryServices()|
            getPrimaryServices('heart_rate')[UUID]
          ]),
          new DOMException(
            'GATT Server is disconnected. ' +
            'Cannot retrieve services. ' +
            '(Re)connect first with `device.gatt.connect`.',
            'NetworkError')));
      });
}, 'Device disconnects before FUNCTION_NAME. Reject with NetworkError.');
