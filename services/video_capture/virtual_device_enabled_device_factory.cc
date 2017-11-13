// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/video_capture/virtual_device_enabled_device_factory.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "media/capture/video/video_capture_device_info.h"
#include "services/video_capture/virtual_device_mojo_adapter.h"

namespace video_capture {

VirtualDeviceEnabledDeviceFactory::VirtualDeviceEntry::VirtualDeviceEntry() =
    default;

VirtualDeviceEnabledDeviceFactory::VirtualDeviceEntry::~VirtualDeviceEntry() =
    default;

VirtualDeviceEnabledDeviceFactory::VirtualDeviceEntry::VirtualDeviceEntry(
    VirtualDeviceEnabledDeviceFactory::VirtualDeviceEntry&& other) = default;

VirtualDeviceEnabledDeviceFactory::VirtualDeviceEntry&
VirtualDeviceEnabledDeviceFactory::VirtualDeviceEntry::operator=(
    VirtualDeviceEnabledDeviceFactory::VirtualDeviceEntry&& other) = default;

VirtualDeviceEnabledDeviceFactory::VirtualDeviceEnabledDeviceFactory(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref,
    std::unique_ptr<mojom::DeviceFactory> device_factory)
    : service_ref_(std::move(service_ref)),
      device_factory_(std::move(device_factory)),
      weak_factory_(this) {}

VirtualDeviceEnabledDeviceFactory::~VirtualDeviceEnabledDeviceFactory() =
    default;

void VirtualDeviceEnabledDeviceFactory::GetDeviceInfos(
    GetDeviceInfosCallback callback) {
  device_factory_->GetDeviceInfos(
      base::BindOnce(&VirtualDeviceEnabledDeviceFactory::OnGetDeviceInfos,
                     weak_factory_.GetWeakPtr(), base::Passed(&callback)));
}

void VirtualDeviceEnabledDeviceFactory::CreateDevice(
    const std::string& device_id,
    mojom::DeviceRequest device_request,
    CreateDeviceCallback callback) {
  auto virtual_device_iter = virtual_devices_by_id_.find(device_id);
  if (virtual_device_iter != virtual_devices_by_id_.end()) {
    VirtualDeviceEntry& device_entry = virtual_device_iter->second;
    // The requested virtual device is already used by another client.
    // Revoke the access for the current client, then bind to the new request.
    if (device_entry.consumer_binding) {
      device_entry.consumer_binding.reset();
      device_entry.device->Stop();
    }
    device_entry.consumer_binding =
        std::make_unique<mojo::Binding<mojom::Device>>(
            device_entry.device.get(), std::move(device_request));
    device_entry.consumer_binding->set_connection_error_handler(
        base::Bind(&VirtualDeviceEnabledDeviceFactory::
                       OnVirtualDeviceConsumerConnectionErrorOrClose,
                   base::Unretained(this), device_id));
    std::move(callback).Run(mojom::DeviceAccessResultCode::SUCCESS);
    return;
  }

  device_factory_->CreateDevice(device_id, std::move(device_request),
                                std::move(callback));
}

void VirtualDeviceEnabledDeviceFactory::AddVirtualDevice(
    const media::VideoCaptureDeviceInfo& device_info,
    mojom::VirtualDeviceRequest virtual_device_request,
    AddVirtualDeviceCallback callback) {
  auto device_id = device_info.descriptor.device_id;
  auto virtual_device_iter = virtual_devices_by_id_.find(device_id);
  if (virtual_device_iter != virtual_devices_by_id_.end()) {
    // Revoke the access for the current producer and consumer by
    // removing it from the list.
    virtual_devices_by_id_.erase(virtual_device_iter);
  }

  VirtualDeviceEntry device_entry;
  device_entry.device = base::MakeUnique<VirtualDeviceMojoAdapter>(
      service_ref_->Clone(), device_info);
  device_entry.producer_binding =
      base::MakeUnique<mojo::Binding<mojom::VirtualDevice>>(
          device_entry.device.get(), std::move(virtual_device_request));
  device_entry.producer_binding->set_connection_error_handler(
      base::Bind(&VirtualDeviceEnabledDeviceFactory::
                     OnVirtualDeviceProducerConnectionErrorOrClose,
                 base::Unretained(this), device_id));
  virtual_devices_by_id_[device_id] = std::move(device_entry);

  std::move(callback).Run();
}

void VirtualDeviceEnabledDeviceFactory::OnGetDeviceInfos(
    GetDeviceInfosCallback callback,
    const std::vector<media::VideoCaptureDeviceInfo>& real_device_infos) {
  std::vector<media::VideoCaptureDeviceInfo> device_infos;
  for (const auto& device_entry : virtual_devices_by_id_) {
    device_infos.push_back(device_entry.second.device->device_info());
  }
  device_infos.insert(std::end(device_infos), std::begin(real_device_infos),
                      std::end(real_device_infos));
  std::move(callback).Run(device_infos);
}

void VirtualDeviceEnabledDeviceFactory::
    OnVirtualDeviceProducerConnectionErrorOrClose(
        const std::string& device_id) {
  virtual_devices_by_id_[device_id].device->Stop();
  virtual_devices_by_id_.erase(device_id);
}

void VirtualDeviceEnabledDeviceFactory::
    OnVirtualDeviceConsumerConnectionErrorOrClose(
        const std::string& device_id) {
  virtual_devices_by_id_[device_id].device->Stop();
}

}  // namespace video_capture
