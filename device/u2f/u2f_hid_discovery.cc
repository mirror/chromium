// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_hid_discovery.h"

#include <utility>

#include "base/bind.h"
#include "device/base/device_client.h"
#include "device/u2f/u2f_hid_device.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/device/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace device {

U2fHidDiscovery::U2fHidDiscovery(service_manager::Connector* connector)
    : connector_(connector), binding_(this), weak_factory_(this) {
  filter_.SetUsagePage(0xf1d0);
}

U2fHidDiscovery::~U2fHidDiscovery() = default;

void U2fHidDiscovery::StartImpl(StartedCallback started) {
  DCHECK(connector_);
  connector_->BindInterface(device::mojom::kServiceName,
                            mojo::MakeRequest(&hid_manager_));
  device::mojom::HidManagerClientAssociatedPtrInfo client;
  binding_.Bind(mojo::MakeRequest(&client));

  hid_manager_->GetDevicesAndSetClient(
      std::move(client),
      base::BindOnce(&U2fHidDiscovery::OnGetDevices, weak_factory_.GetWeakPtr(),
                     std::move(started)));
}
void U2fHidDiscovery::StopImpl(StoppedCallback stopped) {
  binding_.Unbind();
  std::move(stopped).Run();
}

void U2fHidDiscovery::DeviceAdded(device::mojom::HidDeviceInfoPtr device_info) {
  // Ignore non-U2F devices
  if (filter_.Matches(*device_info)) {
    callback_.Run(std::make_unique<U2fHidDevice>(std::move(device_info),
                                                 hid_manager_.get()),
                  DeviceStatus::ADDED);
  }
}

void U2fHidDiscovery::DeviceRemoved(
    device::mojom::HidDeviceInfoPtr device_info) {
  // Ignore non-U2F devices
  if (filter_.Matches(*device_info)) {
    callback_.Run(std::make_unique<U2fHidDevice>(std::move(device_info),
                                                 hid_manager_.get()),
                  DeviceStatus::REMOVED);
  }
}

void U2fHidDiscovery::OnGetDevices(
    StartedCallback started,
    std::vector<device::mojom::HidDeviceInfoPtr> device_infos) {
  for (auto& device_info : device_infos) {
    if (filter_.Matches(*device_info)) {
      callback_.Run(std::make_unique<U2fHidDevice>(std::move(device_info),
                                                   hid_manager_.get()),
                    DeviceStatus::KNOWN);
    }
  }

  std::move(started).Run(true);
}

}  // namespace device
