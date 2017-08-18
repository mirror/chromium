// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/multidevice.h"
#include "base/logging.h"
#include "components/multidevice/service/device_sync_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace multidevice {

MultiDevice::MultiDevice() : weak_ptr_factory_(this) {
  LOG(ERROR) << "MultiDevice Constructor called!";
}

MultiDevice::~MultiDevice() {}

void MultiDevice::OnStart() {
  ref_factory_ = base::MakeUnique<service_manager::ServiceContextRefFactory>(
      base::Bind(&service_manager::ServiceContext::RequestQuit,
                 base::Unretained(context())));
  registry_.AddInterface<device_sync::mojom::DeviceSync>(
      base::Bind(&MultiDevice::CreateDeviceSyncImpl, base::Unretained(this),
                 ref_factory_.get()));
}

void MultiDevice::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void MultiDevice::CreateDeviceSyncImpl(
    service_manager::ServiceContextRefFactory* ref_factory,
    device_sync::mojom::DeviceSyncRequest request) {
  mojo::MakeStrongBinding(
      base::MakeUnique<DeviceSyncImpl>(ref_factory->CreateRef()),
      std::move(request));
}

}  // namespace multidevice