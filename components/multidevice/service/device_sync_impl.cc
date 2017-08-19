// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/device_sync_impl.h"

#include "base/logging.h"

namespace multidevice {

DeviceSyncImpl::DeviceSyncImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)) {}

DeviceSyncImpl::~DeviceSyncImpl() {}

void DeviceSyncImpl::ForceEnrollmentNow(ForceEnrollmentNowCallback callback) {
  listeners_.ForAllPtrs([](device_sync::mojom::DeviceSyncObserver*
                               listener) {  // LOG(ERROR) << "c'mon now";
    listener->OnEnrollmentFinished(true /* success */);
  });
  std::move(callback).Run(device_sync::mojom::ResultCode::SUCCESS);
}

void DeviceSyncImpl::ForceSyncNow() {}

void DeviceSyncImpl::AddObserver(
    device_sync::mojom::DeviceSyncObserverPtr observer) {
  listeners_.AddPtr(std::move(observer));
}

}  // namespace multidevice