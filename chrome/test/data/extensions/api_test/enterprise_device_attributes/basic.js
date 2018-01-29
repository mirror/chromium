// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Must be packed to ../enterprise_device_attributes.crx using the private key
// ../enterprise_device_attributes.pem .

var assertEq = chrome.test.assertEq;
var assertTrue = chrome.test.assertTrue;
var callbackPass = chrome.test.callbackPass
var succeed = chrome.test.succeed;

chrome.test.getConfig(function(config) {
  var customArg = JSON.parse(config.customArg);
  var expectedDirectoryDeviceId = customArg.expectedDirectoryDeviceId;
  var expectedSerialNumber = customArg.expectedSerialNumber;
  var expectedAssetId = customArg.expectedAssetId;

  chrome.enterprise.deviceAttributes.getDirectoryDeviceId(
      callbackPass(function(deviceId) {
        assertEq(expectedDirectoryDeviceId, deviceId);
      }));

  chrome.enterprise.deviceAttributes.getDeviceSerialNumber(
      callbackPass(function(serialNumber) {
        assertEq(expectedSerialNumber, serialNumber);
      }));

  chrome.enterprise.deviceAttributes.getDeviceAssetId(
      callbackPass(function(assetId) {
        assertEq(expectedAssetId, assetId);
      }));

  succeed();
});
