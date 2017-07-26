// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/serial/serial_device_enumerator_impl.h"

#include "base/memory/ptr_util.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace device {

// static
void SerialDeviceEnumeratorImpl::Create(
    mojom::SerialDeviceEnumeratorRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<SerialDeviceEnumeratorImpl>(),
                          std::move(request));
}

SerialDeviceEnumeratorImpl::SerialDeviceEnumeratorImpl()
    : enumerator_(device::SerialDeviceEnumerator::Create()) {}

SerialDeviceEnumeratorImpl::~SerialDeviceEnumeratorImpl() = default;

void SerialDeviceEnumeratorImpl::GetDevices(GetDevicesCallback callback) {
  DCHECK(enumerator_);
  std::move(callback).Run(enumerator_->GetDevices());
}

}  // namespace device
