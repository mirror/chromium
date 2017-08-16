// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/device_sync_impl.h"

#include "base/logging.h"

namespace multidevice {

DeviceSyncImpl::DeviceSyncImpl() {}

DeviceSyncImpl::~DeviceSyncImpl() {}

void DeviceSyncImpl::ForceEnrollmentNow() {
  LOG(ERROR) << "This is not compiling...";
  listeners_.ForEach([](db::mojom::DeviceSync* listener) {
    listener->OnEnrollmentFinished(true /* success */);
  });
}

void DeviceSyncImpl::ForceSyncNow() {}

void DeviceSyncImpl::AddObserver(
    device_sync::mojom::DeviceSyncObserverPtr observer) {
  listeners_.AddPtr(std::move(observer));
}

}  // namespace multidevice