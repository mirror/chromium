// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_MOCK_U2F_DISCOVERY_H_
#define DEVICE_U2F_MOCK_U2F_DISCOVERY_H_

#include <memory>

#include "device/u2f/u2f_device.h"
#include "device/u2f/u2f_discovery.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace device {

class MockU2fDiscovery : public U2fDiscovery {
 public:
  MockU2fDiscovery();
  ~MockU2fDiscovery() override;

  MOCK_METHOD1(StartImpl, void(StartedCallback));
  MOCK_METHOD1(StopImpl, void(StoppedCallback));

  void DiscoverDevice(std::unique_ptr<U2fDevice> device, DeviceStatus status);

  static void StartSuccess(StartedCallback started);
  static void StartFailure(StartedCallback started);

  static void StartSuccessAsync(StartedCallback started);
  static void StartFailureAsync(StartedCallback started);

  DeviceStatusCallback callback() const { return callback_; }
};

}  // namespace device

#endif  // DEVICE_U2F_MOCK_U2F_DISCOVERY_H_
