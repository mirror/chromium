// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/test/test_device_status_listener.h"

#include "components/download/internal/scheduler/network_status_listener.h"

namespace download {
namespace test {

TestDeviceStatusListener::TestDeviceStatusListener(const base::TimeDelta& delay)
    : DeviceStatusListener(delay) {}

void TestDeviceStatusListener::BuildNetworkStatusListener() {
  network_listener_ = base::MakeUnique<NetworkStatusListenerImpl>();
}

}  // namespace test
}  // namespace download
