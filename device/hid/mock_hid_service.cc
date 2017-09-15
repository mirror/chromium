// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>

#include "device/hid/mock_hid_connection.h"
#include "device/hid/mock_hid_service.h"

namespace device {

MockHidService::MockHidService() : weak_factory_(this) {}

MockHidService::~MockHidService() = default;

base::WeakPtr<HidService> MockHidService::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void MockHidService::AddDevice(scoped_refptr<HidDeviceInfo> info) {
  HidService::AddDevice(info);
}

void MockHidService::RemoveDevice(
    const HidPlatformDeviceId& platform_device_id) {
  HidService::RemoveDevice(platform_device_id);
}

void MockHidService::FirstEnumerationComplete() {
  HidService::FirstEnumerationComplete();
}

void MockHidService::Connect(const std::string& device_id,
                             const ConnectCallback& callback) {
  const auto& map_entry = devices().find(device_id);
  if (map_entry == devices().end()) {
    callback.Run(nullptr);
    return;
  }

  scoped_refptr<HidDeviceInfo> device_info = map_entry->second;
  scoped_refptr<MockHidConnection> mock_connection =
      new MockHidConnection(device_info);
  callback.Run(mock_connection);
}

const std::map<std::string, scoped_refptr<HidDeviceInfo>>&
MockHidService::devices() const {
  return HidService::devices();
}

}  // namespace device
