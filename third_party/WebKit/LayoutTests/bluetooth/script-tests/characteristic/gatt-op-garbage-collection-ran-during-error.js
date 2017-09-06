'use strict';
let test_desc = 'Garbage Collection ran during a FUNCTION_NAME call that '
   'fails. Should not crash.'
let characteristic, fake_characteristic, func_promise;
promise_test(() => getMeasurementIntervalCharacteristic()
  .then(_ => ({characteristic, fake_characteristic} = _))
  // 1. Set up error responses.
  .then(() => fake_characteristic.setNextReadResponse(1))
  .then(() => fake_characteristic.setNextWriteResponse(1))
  .then(() => fake_characteristic.setNextSubscribeToNotificationsResponse(1))
  .then(() => {
    // 2. Begin making a call that will fail.
    func_promise = assert_promise_rejects_with_message(
        characteristic.CALLS([
            readValue()|
            writeValue(new Uint8Array(1))|
            startNotifications()]),
        new DOMException(
            'GATT Server is disconnected. Cannot perform GATT operations. ' +
            '(Re)connect first with `device.gatt.connect`.',
            'NetworkError'));
    // 3. Disconnect called to clear attributeInstanceMap and allow the object
    // to get garbage collected.
    characteristic.service.device.gatt.disconnect();
    characteristic = undefined;
    fake_characteristic = undefined;
  })
  // 4. Run garbage collection.
  .then(runGarbageCollection)
  .then(() => func_promise), test_desc);
