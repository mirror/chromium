// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_hid_discovery.h"

#include <utility>

#include "base/bind.h"
#include "device/base/device_client.h"
#include "device/u2f/u2f_hid_device.h"

namespace device {

U2fHidDiscovery::U2fHidDiscovery()
    : hid_service_observer_(this), weak_factory_(this) {
  filter_.SetUsagePage(0xf1d0);
}

U2fHidDiscovery::~U2fHidDiscovery() = default;

void U2fHidDiscovery::StartImpl(StartedCallback started) {
  hid_service_ = DeviceClient::Get()->GetHidService();
  DCHECK(hid_service_);
  hid_service_->GetDevices(base::Bind(&U2fHidDiscovery::OnGetDevices,
                                      weak_factory_.GetWeakPtr(),
                                      std::move(started)));
}

void U2fHidDiscovery::StopImpl(StoppedCallback stopped) {
  std::move(stopped).Run();
  hid_service_observer_.Remove(hid_service_);
}

void U2fHidDiscovery::OnDeviceAdded(
    device::mojom::HidDeviceInfoPtr device_info) {
  if (filter_.Matches(*device_info)) {
    callback_.Run(std::make_unique<U2fHidDevice>(std::move(device_info)),
                  DeviceStatus::ADDED);
  }
}

void U2fHidDiscovery::OnDeviceRemoved(
    device::mojom::HidDeviceInfoPtr device_info) {
  if (filter_.Matches(*device_info)) {
    callback_.Run(std::make_unique<U2fHidDevice>(std::move(device_info)),
                  DeviceStatus::REMOVED);
  }
}

void U2fHidDiscovery::OnGetDevices(
    StartedCallback started,
    std::vector<device::mojom::HidDeviceInfoPtr> device_infos) {
  for (auto& device_info : device_infos) {
    if (filter_.Matches(*device_info)) {
      callback_.Run(std::make_unique<U2fHidDevice>(std::move(device_info)),
                    DeviceStatus::KNOWN);
    }
  }

  std::move(started).Run(true);
  hid_service_observer_.Add(hid_service_);
}

}  // namespace device
