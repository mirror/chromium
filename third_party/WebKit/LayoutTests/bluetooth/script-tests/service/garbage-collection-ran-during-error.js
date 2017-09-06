'use strict';
let test_desc = 'Garbage Collection ran during FUNCTION_NAME call that ' +
   'fails. Should not crash';
let service;
promise_test(() => getHealthThermometerService()
  .then(_ => ({service} = _))
  .then(() => service.device.gatt.disconnect())
  .then(runGarbageCollection)
  .then(() => assert_promise_rejects_with_message(
      service.CALLS([
          getCharacteristic('measurement_interval')|
          getCharacteristics()|
          getCharacteristics('measurement_interval')[UUID]]),
      new DOMException(
          'GATT Server is disconnected. Cannot retrieve characteristics. ' +
          '(Re)connect first with `device.gatt.connect`.',
          'NetworkError'))), test_desc);
