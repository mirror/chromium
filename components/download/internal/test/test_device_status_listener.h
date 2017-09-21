// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_TEST_TEST_DEVICE_STATUS_LISTENER_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_TEST_TEST_DEVICE_STATUS_LISTENER_H_

#include "components/download/internal/scheduler/device_status_listener.h"

namespace download {
namespace test {

// Used in tests which only loads the default NetworkStatusListener.
class TestDeviceStatusListener : public DeviceStatusListener {
 public:
  explicit TestDeviceStatusListener(const base::TimeDelta& delay);

  // DeviceStatusListener implementation.
  void BuildNetworkStatusListener() override;
};

}  // namespace test
}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_TEST_TEST_DEVICE_STATUS_LISTENER_H_
