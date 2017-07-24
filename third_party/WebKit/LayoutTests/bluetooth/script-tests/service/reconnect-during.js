'use strict';
promise_test(() => {
  let device, service, characteristic;
  return getHealthThermometerDeviceWithServicesDiscovered({
      filters: [{services: [health_thermometer.name]}],
  })
    .then(_ => ({device, service, characteristic} = _))
    .then(() => assert_class_string(
        characteristic, 'BluetoothRemoteGATTCharacteristic'))
    .then(() => {
      let promise = assert_promise_rejects_with_message(service.CALLS([
        getCharacteristic(measurement_interval.name)|
        getCharacteristics()|
        getCharacteristics(measurement_interval.name)[UUID]
      ]), new DOMException(
          'GATT Server is disconnected. Cannot retrieve characteristics. ' +
          '(Re)connect first with `device.gatt.connect`.',
          'NetworkError'));
      // connect() guarantees on OS-level connection, but disconnect()
      // only disconnects the current instance.
      // getHealthThermometerDeviceWithServicesDiscovered holds another
      // connection in an iframe, so disconnect() and connect() are certain to
      // reconnect.  However, disconnect() will invalidate the service object so
      // the subsequent calls made to it will fail, even after reconnecting.
      device.gatt.disconnect();
      return device.gatt.connect().then(() => promise);
    });
}, 'disconnect() and connect() called during FUNCTION_NAME. Reject with ' +
   'NetworkError.');
