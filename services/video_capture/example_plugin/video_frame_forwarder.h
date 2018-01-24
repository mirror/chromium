// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIDEO_CAPTURE_EXAMPLE_PLUGIN_VIDEO_FRAME_FORWARDER_H_
#define SERVICES_VIDEO_CAPTURE_EXAMPLE_PLUGIN_VIDEO_FRAME_FORWARDER_H_

#include "media/capture/video/shared_memory_handle_provider.h"
#include "services/video_capture/public/interfaces/producer.mojom.h"
#include "services/video_capture/public/interfaces/receiver.mojom.h"
#include "services/video_capture/public/interfaces/virtual_device.mojom.h"

namespace video_capture {
namespace example_plugin {

// Forwards incoming video frames to a given VirtualDevice.
class VideoFrameForwarder : public mojom::Receiver {
 public:
  // Delegates invocations of the mojom::Producer interface to a
  // VideoFrameForwarder. We use this to resolve naming collisions between
  // methods of mojom::Receiver and mojom::Producer, by forwarding to methods of
  // VideoFrameForwarder with unambiguous names.
  class ProducerDelegator : public mojom::Producer {
   public:
    ProducerDelegator(VideoFrameForwarder* forwarder);

    // mojom::Producer implementation.
    void OnNewBufferHandle(int32_t buffer_id,
                           mojo::ScopedSharedBufferHandle buffer_handle,
                           OnNewBufferHandleCallback callback) override;
    void OnBufferRetired(int32_t buffer_id) override;

   private:
    VideoFrameForwarder* const forwarder_;
  };

  VideoFrameForwarder();
  ~VideoFrameForwarder() override;

  // Must be called after construction and before any of the mojom::Producer or
  // mojom::Receiver methods. Must not be called after that.
  void SetTarget(mojom::VirtualDevicePtr target);
  mojom::Producer* GetAsProducer();

  void OnProducerNewBufferHandle(
      int32_t buffer_id,
      mojo::ScopedSharedBufferHandle buffer_handle,
      mojom::Producer::OnNewBufferHandleCallback callback);
  void OnProducerBufferRetired(int32_t buffer_id);

  // mojom::Receiver implementation.
  void OnNewBufferHandle(int32_t buffer_id,
                         mojo::ScopedSharedBufferHandle buffer_handle) override;
  void OnFrameReadyInBuffer(
      int32_t buffer_id,
      int32_t frame_feedback_id,
      mojom::ScopedAccessPermissionPtr access_permission,
      ::media::mojom::VideoFrameInfoPtr frame_info) override;
  void OnBufferRetired(int32_t buffer_id) override;
  void OnError() override;
  void OnLog(const std::string& message) override;
  void OnStarted() override;
  void OnStartedUsingGpuDecode() override;

 private:
  void CopyFrameToVirtualDevice(
      int32_t incoming_buffer_id,
      mojom::ScopedAccessPermissionPtr incoming_buffer_access_permission,
      media::mojom::VideoFrameInfoPtr frame_info,
      int32_t outgoing_buffer_id);

  mojom::VirtualDevicePtr target_;
  std::map<int32_t, std::unique_ptr<media::SharedMemoryHandleProvider>>
      incoming_buffer_id_to_buffer_map_;
  std::map<int32_t, std::unique_ptr<media::SharedMemoryHandleProvider>>
      outgoing_buffer_id_to_buffer_map_;
  std::unique_ptr<ProducerDelegator> producer_delegator_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace example_plugin
}  // namespace video_capture

#endif  // SERVICES_VIDEO_CAPTURE_EXAMPLE_PLUGIN_VIDEO_FRAME_FORWARDER_H_
