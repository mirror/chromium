// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIDEO_CAPTURE_EXAMPLE_PLUGIN_CONNECTION_HANDLER_H_
#define SERVICES_VIDEO_CAPTURE_EXAMPLE_PLUGIN_CONNECTION_HANDLER_H_

#include "base/threading/thread_checker.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/video_capture/example_plugin/video_frame_forwarder.h"
#include "services/video_capture/public/interfaces/device_factory.mojom.h"
#include "services/video_capture/public/interfaces/producer.mojom.h"
#include "services/video_capture/public/interfaces/receiver.mojom.h"

namespace video_capture {
namespace example_plugin {

// Does something interesting with the video capture service when it connects.
class ConnectionHandler {
 public:
  ConnectionHandler();
  ~ConnectionHandler();

  void OnVideoCaptureServiceConnected(
      video_capture::mojom::DeviceFactoryPtr device_factory);
  void OnVideoCaptureServiceDisconnected();

 private:
  void OpenFirstCameraAndLoopVideoBackIntoNewVirtualDevice();
  void OnDeviceInfosReceived(
      const std::vector<media::VideoCaptureDeviceInfo>& device_infos);
  void OnCreateDeviceReply(media::VideoCaptureFormat format_to_request,
                           mojom::DeviceAccessResultCode result_code);

  mojom::DeviceFactoryPtr device_factory_;
  mojom::DevicePtr device_;
  std::unique_ptr<VideoFrameForwarder> video_frame_forwarder_;
  std::unique_ptr<mojo::Binding<mojom::Receiver>> receiver_binding_;
  std::unique_ptr<mojo::Binding<mojom::Producer>> producer_binding_;

  THREAD_CHECKER(thread_checker_);
};

}  // namespace example_plugin
}  // namespace video_capture

#endif  // SERVICES_VIDEO_CAPTURE_EXAMPLE_PLUGIN_CONNECTION_HANDLER_H_
