// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/geolocation/wifi_polling_policy.h"

namespace device {

namespace {
WifiPollingPolicy* g_wifi_polling_policy = nullptr;
}  // namespace

// static
void WifiPollingPolicy::Initialize(std::unique_ptr<WifiPollingPolicy> policy) {
  g_wifi_polling_policy = policy.release();
}

// static
void WifiPollingPolicy::Shutdown() {
  delete g_wifi_polling_policy;
}

// static
WifiPollingPolicy* WifiPollingPolicy::Get() {
  return g_wifi_polling_policy;
}

// static
bool WifiPollingPolicy::IsInitialized() {
  return g_wifi_polling_policy != nullptr;
}

}  // namespace device
