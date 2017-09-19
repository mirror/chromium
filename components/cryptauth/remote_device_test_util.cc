// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/remote_device_test_util.h"

#include "base/base64.h"

namespace cryptauth {

std::vector<RemoteDevice> GenerateTestRemoteDevices(size_t num_to_create) {
  std::vector<RemoteDevice> generated_devices;

  std::string i_char;
  for (size_t i = 0; i < num_to_create; i++) {
    RemoteDevice device;
    i_char = std::to_string(i);
    device.name = "name" + i_char;
    device.public_key = "publicKey" + i_char;
    device.bluetooth_address = "blueToothAddress" + i_char;
    device.persistent_symmetric_key = "persistentSymmetricKey" + i_char;
    device.unlock_key = rand() % 2;
    device.supports_mobile_hotspot = rand() % 2;
    generated_devices.push_back(device);
  }

  return generated_devices;
}

}  // namespace cryptauth
