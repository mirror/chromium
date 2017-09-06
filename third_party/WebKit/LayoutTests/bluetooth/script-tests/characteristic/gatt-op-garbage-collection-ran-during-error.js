'use strict';
const test_desc = 'Garbage Collection ran during a FUNCTION_NAME call that '
   'fails. Should not crash.'
let test_type = 'CALLS([
  readValue()|
  writeValue()|
  startNotifications()])';
let characteristic, fake_characteristic, func_promise;
promise_test(() => getMeasurementIntervalCharacteristic()
  .then(_ => ({characteristic, fake_characteristic} = _))
  // 1. Set up the error response.
  .then(() => {
    switch (test_type) {
      case 'readValue()':
        return fake_characteristic.setNextReadResponse(GATT_INVALID_HANDLE);
      case 'writeValue()':
        return fake_characteristic.setNextWriteResponse(GATT_INVALID_HANDLE);
      case 'startNotifications()':
        // TODO: startNotifications will error on its own because there is no
        // CCCD set.  When we manage to correct this, we should add the
        // following line:
        // fake_characteristic.setNextSubscribeToNotificationsResponse(
        //    GATT_INVALID_HANDLE);
        break;
      default:
        assert_unreached('Unexpected test type: ' + test_type);
    }
  })
  .then(() => {
    // 2. Begin making a call that will fail.
    let promise;
    switch (test_type) {
      case 'readValue()':
        promise = characteristic.readValue()
        break;
      case 'writeValue()':
        promise = characteristic.writeValue(new Uint8Array(1));
        break;
      case 'startNotifications()':
        promise = characteristic.startNotifications();
        break;
      default:
        assert_unreached('Unexpected test type: ' + test_type);
    }
    func_promise = Promise.all([
      assert_promise_rejects_with_message(
          promise.then(() =>
              assert_unreached('FUNCTION_NAME should not have succeeded')),
          new DOMException(
              'GATT Server is disconnected. Cannot perform GATT operations. ' +
              '(Re)connect first with `device.gatt.connect`.',
              'NetworkError')),
      // 3. Make sure all our responses are consumed.
      fake_characteristic.allResponsesConsumed()
        .then(consumed => assert_true(consumed, 'Response not consumed')),
    ]);
    // 4. Disconnect called to clear attributeInstanceMap and allow the object
    // to get garbage collected.
    characteristic.service.device.gatt.disconnect();
    characteristic = undefined;
    fake_characteristic = undefined;
    // 5. Run garbage collection.
    return runGarbageCollection();
  })
  .then(() => func_promise), test_desc);
