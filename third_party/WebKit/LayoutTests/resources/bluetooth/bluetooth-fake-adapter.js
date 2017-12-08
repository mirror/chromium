// TODO(509038): This is a temporary file that will be used by tests that
// require TestRunner to function properly while the tests are converted to use
// the Web Platform Test API instead. The file should be removed once there are
// no tests that require these TestRunner functions.
'use strict';

function assert_testRunner() {
  assert_true(window.testRunner instanceof Object,
    "window.testRunner is required for this test, it will not work manually.");
}

function setBluetoothManualChooser(enable) {
  assert_testRunner();
  testRunner.setBluetoothManualChooser(enable);
}

// Calls testRunner.getBluetoothManualChooserEvents() until it's returned
// |expected_count| events. Or just once if |expected_count| is undefined.
function getBluetoothManualChooserEvents(expected_count) {
  assert_testRunner();
  return new Promise((resolve, reject) => {
    let events = [];
    let accumulate_events = new_events => {
      events.push(...new_events);
      if (events.length >= expected_count) {
        resolve(events);
      } else {
        testRunner.getBluetoothManualChooserEvents(accumulate_events);
      }
    };
    testRunner.getBluetoothManualChooserEvents(accumulate_events);
  });
}

function sendBluetoothManualChooserEvent(event, argument) {
  assert_testRunner();
  testRunner.sendBluetoothManualChooserEvent(event, argument);
}

function setBluetoothFakeAdapter(adapter_name) {
  assert_testRunner();
  return new Promise(resolve => {
    testRunner.setBluetoothFakeAdapter(adapter_name, resolve);
  });
}

// Parses add-device(name)=id lines in
// testRunner.getBluetoothManualChooserEvents() output, and exposes the name->id
// mapping.
class AddDeviceEventSet {
  constructor() {
    this._idsByName = new Map();
    this._addDeviceRegex = /^add-device\(([^)]+)\)=(.+)$/;
  }
  assert_add_device_event(event, description) {
    let match = this._addDeviceRegex.exec(event);
    assert_true(!!match, event + " isn't an add-device event: " + description);
    this._idsByName.set(match[1], match[2]);
  }
  has(name) {
    return this._idsByName.has(name);
  }
  get(name) {
    return this._idsByName.get(name);
  }
}
