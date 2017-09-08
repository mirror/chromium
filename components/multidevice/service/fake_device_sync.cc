// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/fake_device_sync.h"

namespace multidevice {

// static
FakeDeviceSync::Factory* FakeDeviceSync::Factory::factory_instance_ = nullptr;

// static
std::unique_ptr<FakeDeviceSync> FakeDeviceSync::Factory::NewInstance(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref) {
  if (!factory_instance_) {
    factory_instance_ = new Factory();
  }
  return factory_instance_->BuildInstance(std::move(service_ref));
}

// static
void FakeDeviceSync::Factory::SetInstanceForTesting(Factory* factory) {
  factory_instance_ = factory;
}

std::unique_ptr<FakeDeviceSync> FakeDeviceSync::Factory::BuildInstance(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref) {
  return base::WrapUnique(new FakeDeviceSync(std::move(service_ref)));
}

FakeDeviceSync::FakeDeviceSync(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)) {}

FakeDeviceSync::~FakeDeviceSync() {}

void FakeDeviceSync::ForceEnrollmentNow() {
  observers_.ForAllPtrs([](device_sync::mojom::DeviceSyncObserver* observer) {
    observer->OnEnrollmentFinished(true /* success */);
  });
}

void FakeDeviceSync::ForceSyncNow() {
  observers_.ForAllPtrs([](device_sync::mojom::DeviceSyncObserver* observer) {
    observer->OnDevicesSynced(true /* success */);
  });
}

void FakeDeviceSync::AddObserver(
    device_sync::mojom::DeviceSyncObserverPtr observer) {
  observers_.AddPtr(std::move(observer));
}

}  // namespace multidevice