// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/video_capture/example_plugin/connection_handler.h"

#include "base/bind.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace video_capture {
namespace example_plugin {

ConnectionHandler::ConnectionHandler() = default;

ConnectionHandler::~ConnectionHandler() = default;

void ConnectionHandler::OnVideoCaptureServiceConnected(
    video_capture::mojom::DeviceFactoryPtr device_factory) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  device_factory_ = std::move(device_factory);
  device_factory_.set_connection_error_handler(
      base::BindOnce(&ConnectionHandler::OnVideoCaptureServiceDisconnected,
                     base::Unretained(this)));
  OpenFirstCameraAndLoopVideoBackIntoNewVirtualDevice();
}

void ConnectionHandler::OnVideoCaptureServiceDisconnected() {}

void ConnectionHandler::OpenFirstCameraAndLoopVideoBackIntoNewVirtualDevice() {
  device_factory_->GetDeviceInfos(base::BindOnce(
      &ConnectionHandler::OnDeviceInfosReceived, base::Unretained(this)));
}

void ConnectionHandler::OnDeviceInfosReceived(
    const std::vector<media::VideoCaptureDeviceInfo>& device_infos) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOG(3) << "Received " << device_infos.size() << " device infos";
  if (device_infos.empty()) {
    LOG(ERROR) << "No device infos received.";
    return;
  }
  const media::VideoCaptureDeviceInfo& device_info = device_infos.back();
  if (device_info.supported_formats.empty()) {
    LOG(ERROR) << "No supported formats.";
    return;
  }
  media::VideoCaptureFormat format_to_request =
      device_info.supported_formats.front();
  VLOG(3) << "Creating Device " << device_info.descriptor.display_name;
  device_factory_->CreateDevice(
      device_info.descriptor.device_id, mojo::MakeRequest(&device_),
      video_capture::mojom::AccessRequestType::
          DISALLOW_PREEMPTION_AND_HIDE_FROM_DEVICE_INFOS,
      base::BindOnce(&ConnectionHandler::OnCreateDeviceReply,
                     base::Unretained(this), format_to_request));
}

void ConnectionHandler::OnCreateDeviceReply(
    media::VideoCaptureFormat format_to_request,
    mojom::DeviceAccessResultCode result_code) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (result_code != mojom::DeviceAccessResultCode::SUCCESS) {
    LOG(ERROR) << "Creating device failed";
    return;
  }

  VLOG(3) << "Creating device succeeded";
  device_.set_connection_error_handler(
      base::BindOnce([]() { LOG(WARNING) << "Connection to Device lost"; }));

  video_frame_forwarder_ = std::make_unique<VideoFrameForwarder>();
  mojom::ReceiverPtr forwarder_as_receiver;
  receiver_binding_ = std::make_unique<mojo::Binding<mojom::Receiver>>(
      video_frame_forwarder_.get(), mojo::MakeRequest(&forwarder_as_receiver));
  mojom::ProducerPtr forwarder_as_producer;
  producer_binding_ = std::make_unique<mojo::Binding<mojom::Producer>>(
      video_frame_forwarder_->GetAsProducer(),
      mojo::MakeRequest(&forwarder_as_producer));

  media::VideoCaptureDeviceInfo virtual_device_info;
  virtual_device_info.descriptor.display_name = "Example Virtual Device";
  virtual_device_info.descriptor.device_id = "example_virtual_device";
  mojom::VirtualDevicePtr virtual_device;
  device_factory_->AddVirtualDevice(virtual_device_info,
                                    std::move(forwarder_as_producer),
                                    mojo::MakeRequest(&virtual_device));
  video_frame_forwarder_->SetTarget(std::move(virtual_device));

  VLOG(3) << "Starting device";
  media::VideoCaptureParams requested_settings;
  requested_settings.requested_format = format_to_request;
  device_->Start(requested_settings, std::move(forwarder_as_receiver));
}

}  // namespace example_plugin
}  // namespace video_capture
