// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device_sync_impl.h"

namespace multidevice {

DeviceSyncImpl::DeviceSyncImpl() {}

DeviceSyncImpl::~DeviceSyncImpl() = default;

void DeviceSyncImpl::ForceEnrollmentNow() {
  listerners_.ForEach([](db::mojom::DeviceSync* listener) {
    listener->OnEnrollmentFinished(true /* success */);
  });
}

void DeviceSyncImpl::ForceSyncNow() {}

void DeviceSyncImpl::AddObserver(
    device_sync::mojom::DeviceSyncObserverPtr observer) {
  listerners_.AddPtr(std::move(observer));
}

}  // namespace multidevice