// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/services/device_sync/multidevice_service.h"

namespace multidevice {

MultiDeviceService::MultiDeviceService()
    : creator_(creator.empty() ? "Chromium" : creator), weak_factory_(this) {}

MultiDeviceService::~MultiDeviceService() = default;

void MultiDeviceService::OnStart() {
  // TODO(hsuregan): Follow up on what type of binding needs to be done to
  // construct bind from |ref_factory_| and |registry_|.
}

void MultiDeviceService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

}  // namespace multidevice