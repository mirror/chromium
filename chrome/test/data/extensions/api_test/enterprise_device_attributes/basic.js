// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Must be packed to ../enterprise_device_attributes.crx using the private key
// ../enterprise_device_attributes.pem .

chrome.test.getConfig(function(config) {
  var customArg = JSON.parse(config.customArg);
  var expectedDirectoryDeviceId = customArg.expectedDirectoryDeviceId;
  var expectedSerialNumber = customArg.expectedSerialNumber;
  var expectedAssetId = customArg.expectedAssetId;

  chrome.test.runTests([
    function testDirectoryDeviceId() {
      chrome.enterprise.deviceAttributes.getDirectoryDeviceId(
          chrome.test.callbackPass(function(deviceId) {
            chrome.test.assertEq(expectedDirectoryDeviceId, deviceId);
          }));
    },
    function testDeviceSerialNumber() {
      chrome.enterprise.deviceAttributes.getDeviceSerialNumber(
          chrome.test.callbackPass(function(serialNumber) {
            chrome.test.assertEq(expectedSerialNumber, serialNumber);
          }));

    },
    function testDeviceAssetId() {
      chrome.enterprise.deviceAttributes.getDeviceAssetId(
          chrome.test.callbackPass(function(assetId) {
            chrome.test.assertEq(expectedAssetId, assetId);
          }));
    }
  ]);
});
