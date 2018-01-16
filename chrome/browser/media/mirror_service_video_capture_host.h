// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_MIRROR_SERVICE_VIDEO_CAPTURE_HOST_H_
#define CHROME_BROWSER_MEDIA_MIRROR_SERVICE_VIDEO_CAPTURE_HOST_H_

#include <string>

#include "base/callback.h"
#include "content/browser/renderer_host/media/video_capture_controller_event_handler.h"
#include "content/common/video_capture.mojom.h"
#include "content/public/common/media_stream_request.h"
#include "media/mojo/interfaces/mirror_service_controller.mojom.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace content {
class VideoCaptureProvider;
class VideoCaptureController;
}  // namespace content

namespace media {

class MirrorServiceVideoCaptureHost final
    : public content::VideoCaptureControllerEventHandler,
      public content::mojom::VideoCaptureHost {
 public:
  MirrorServiceVideoCaptureHost(
      const std::string& device_id,
      content::MediaStreamType type,
      const VideoCaptureParams& params,
      scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner);
  ~MirrorServiceVideoCaptureHost() override;

  static void Create(
      const std::string& device_id,
      content::MediaStreamType type,
      const VideoCaptureParams& params,
      scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
      base::OnceCallback<void(content::mojom::VideoCaptureHostPtr)> callback);

  // content::mojom::VideoCaptureHost implementation
  void Start(int32_t device_id,
             int32_t session_id,
             const media::VideoCaptureParams& params,
             content::mojom::VideoCaptureObserverPtr observer) override;
  void Stop(int32_t device_id) override;
  void Pause(int32_t device_id) override;
  void Resume(int32_t device_id,
              int32_t session_id,
              const media::VideoCaptureParams& params) override;
  void RequestRefreshFrame(int32_t device_id) override;
  void ReleaseBuffer(int32_t device_id,
                     int32_t buffer_id,
                     double consumer_resource_utilization) override;
  void GetDeviceSupportedFormats(
      int32_t device_id,
      int32_t session_id,
      GetDeviceSupportedFormatsCallback callback) override;
  void GetDeviceFormatsInUse(int32_t device_id,
                             int32_t session_id,
                             GetDeviceFormatsInUseCallback callback) override;

  // VideoCaptureControllerEventHandler implementations.
  void OnError(content::VideoCaptureControllerID id) override;
  void OnBufferCreated(content::VideoCaptureControllerID id,
                       mojo::ScopedSharedBufferHandle handle,
                       int length,
                       int buffer_id) override;
  void OnBufferDestroyed(content::VideoCaptureControllerID id,
                         int buffer_id) override;
  void OnBufferReady(
      content::VideoCaptureControllerID id,
      int buffer_id,
      const media::mojom::VideoFrameInfoPtr& frame_info) override;
  void OnEnded(content::VideoCaptureControllerID id) override;
  void OnStarted(content::VideoCaptureControllerID id) override;
  void OnStartedUsingGpuDecode(content::VideoCaptureControllerID id) override;

 private:
  const std::string device_id_;
  const content::MediaStreamType type_;
  const VideoCaptureParams params_;
  scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner_;
  std::unique_ptr<content::VideoCaptureProvider> video_capture_provider_;
  scoped_refptr<content::VideoCaptureController> video_capture_controller_;
  content::mojom::VideoCaptureObserverPtr observer_;

  DISALLOW_COPY_AND_ASSIGN(MirrorServiceVideoCaptureHost);
};

}  // namespace media

#endif  // CHROME_BROWSER_MEDIA_MIRROR_SERVICE_VIDEO_CAPTURE_HOST_H_
