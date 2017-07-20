// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var device_;

function getDevice(devices) {
  for (let i = 0; i< devices.length; i++) {
    // HidApiTest.GetUserSelectedDevices adds/removes the device in which
    // the productId is 0x58F1.
    if (devices[i].productId === 0x58F1) {
       return devices[i];
    }
  }
  return undefined;
}

chrome.test.runWithUserGesture(function() {
  let filters = [{'vendorId': 0x18D1, 'productId': 0x58F0},
                 {'vendorId': 0x18D1, 'productId': 0x58F1}];
  chrome.hid.getDevices({'filters':filters}, function(devices) {
    chrome.test.assertNoLastError();
    chrome.test.assertEq(3, devices.length);
    device_ = getDevice(devices);
    chrome.hid.getUserSelectedDevices({'multiple': false}, function(devices) {
      chrome.test.assertNoLastError();
      chrome.test.assertEq(1, devices.length);
      chrome.hid.connect(devices[0].deviceId, function(connection) {
        chrome.test.assertNoLastError();
        chrome.hid.disconnect(connection.connectionId);
        chrome.test.sendMessage("opened_device");
      });
    });
  });
});

chrome.hid.onDeviceRemoved.addListener(function(deviceId) {
  chrome.test.assertEq(device_.deviceId, deviceId);
  chrome.test.sendMessage("removed");
});

chrome.hid.onDeviceAdded.addListener(function(device) {
  chrome.test.assertTrue(device_.deviceId != device.deviceId);
  chrome.test.assertEq(device_.vendorId, device.vendorId);
  chrome.test.sendMessage("added");
});
