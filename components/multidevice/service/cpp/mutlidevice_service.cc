// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/services/device_sync/multidevice_service.h"

namespace multidevice {

MultiDeviceService::MultiDeviceService() : weak_ptr_factory_(this) {}

MultiDeviceService::~MultiDeviceService() {}

void MultiDeviceService::OnStart() {
  ref_factory_ = base::MakeUnique<service_manager::ServiceContextRefFactory>(
      base::Bind(&service_manager::ServiceContext::RequestQuit,
                 base::Unretained(context())));
  registry_.AddInterface<mojom::DeviceSync>(
      base::Bind(&MultiDeviceService::CreateDeviceSyncImpl,
                 base::Unretained(this), ref_factory_.get()));
}

void MultiDeviceService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void MultiDeviceService::CreateDeviceSyncImpl(
    service_manager::ServiceContextRefFactory* ref_factory,
    mojom::DeviceSyncRequest request) {
  mojo::MakeStrongBinding(
      base::MakeUnique<DeviceSyncImplImpl>(ref_factory->CreateRef()),
      std::move(request));
}

}  // namespace multidevice