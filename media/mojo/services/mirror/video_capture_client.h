// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MIRROR_VIDEO_CAPTURE_CLIENT_H_
#define MEDIA_MOJO_SERVICES_MIRROR_VIDEO_CAPTURE_CLIENT_H_

#include "base/callback.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "content/common/media/video_capture.h"
#include "content/common/video_capture.mojom.h"
#include "media/mojo/interfaces/mirror_service_controller.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {

class VideoFrame;

class VideoCaptureClient : public content::mojom::VideoCaptureObserver {
 public:
  VideoCaptureClient(content::mojom::VideoCaptureHostPtr host,
                     base::Callback<void(content::VideoCaptureState)> callback);

  ~VideoCaptureClient() override;

  // content::mojom::VideoCaptureObserver implementations.
  void OnStateChanged(content::mojom::VideoCaptureState state) override;
  void OnBufferCreated(int32_t buffer_id,
                       mojo::ScopedSharedBufferHandle handle) override;
  void OnBufferReady(int32_t buffer_id, mojom::VideoFrameInfoPtr info) override;
  void OnBufferDestroyed(int32_t buffer_id) override;

  using VideoCaptureDeliverFrameCB =
      base::Callback<void(const scoped_refptr<VideoFrame>& video_frame,
                          base::TimeTicks estimated_capture_time)>;
  void Start(VideoCaptureDeliverFrameCB deliver_cb);
  void Stop();
  void SuspendCapture(bool suspend);
  void RequestRefreshFrame();

 private:
  class ClientBuffer;

  content::mojom::VideoCaptureHost* GetVideoCaptureHost();

  void StopDevice();

  void StartCaptureInternal();

  void OnClientBufferFinished(int buffer_id,
                              scoped_refptr<ClientBuffer> buffer,
                              double consumer_resource_utilization);

  using BufferFinishedCallback =
      base::Callback<void(double consumer_resource_utilization)>;
  static void DidFinishConsumingFrame(
      const media::VideoFrameMetadata* metadata,
      const BufferFinishedCallback& callback_to_io_thread);

  const int device_id_ = 0;
  base::Callback<void(content::VideoCaptureState)> state_change_cb_;

  content::mojom::VideoCaptureHostPtrInfo video_capture_host_info_;
  content::mojom::VideoCaptureHostPtr video_capture_host_;
  mojo::Binding<content::mojom::VideoCaptureObserver> observer_binding_;

  using ClientBufferMap = std::map<int32_t, scoped_refptr<ClientBuffer>>;
  ClientBufferMap client_buffers_;
  base::TimeTicks first_frame_ref_time_;
  content::VideoCaptureState state_ = content::VIDEO_CAPTURE_STATE_STOPPED;
  base::ThreadChecker thread_checker_;
  VideoCaptureDeliverFrameCB deliver_frame_cb_;

  base::WeakPtrFactory<VideoCaptureClient> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(VideoCaptureClient);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MIRROR_VIDEO_CAPTURE_CLIENT_H_
