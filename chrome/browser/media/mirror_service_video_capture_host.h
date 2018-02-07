// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_MIRROR_SERVICE_VIDEO_CAPTURE_HOST_H_
#define CHROME_BROWSER_MEDIA_MIRROR_SERVICE_VIDEO_CAPTURE_HOST_H_

#include <string>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/memory/scoped_refptr.h"
#include "content/public/browser/video_capture_device_launcher.h"
#include "content/public/common/media_stream_request.h"
#include "media/capture/mojo/video_capture.mojom.h"
#include "media/capture/video/video_frame_receiver.h"
#include "media/mojo/interfaces/mirror_service_controller.mojom.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace media {

class MirrorServiceVideoCaptureHost final
    : public content::VideoCaptureDeviceLauncher::Callbacks,
      public mojom::VideoCaptureHost,
      public media::VideoFrameReceiver {
 public:
  MirrorServiceVideoCaptureHost(
      const std::string& device_id,
      content::MediaStreamType type,
      const VideoCaptureParams& params,
      scoped_refptr<base::SingleThreadTaskRunner> device_task_runner);
  ~MirrorServiceVideoCaptureHost() override;

  static void Create(
      const std::string& device_id,
      content::MediaStreamType type,
      const VideoCaptureParams& params,
      scoped_refptr<base::SingleThreadTaskRunner> device_task_runner,
      base::OnceCallback<void(mojom::VideoCaptureHostPtr)> callback);

  // mojom::VideoCaptureHost implementations
  void Start(int32_t device_id,
             int32_t session_id,
             const media::VideoCaptureParams& params,
             mojom::VideoCaptureObserverPtr observer) override;
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

  // media::VideoFrameReceiver implementations
  using Buffer = VideoCaptureDevice::Client::Buffer;
  void OnNewBufferHandle(
      int buffer_id,
      std::unique_ptr<media::VideoCaptureDevice::Client::Buffer::HandleProvider>
          handle_provider) override;
  void OnFrameReadyInBuffer(
      int buffer_id,
      int frame_feedback_id,
      std::unique_ptr<
          media::VideoCaptureDevice::Client::Buffer::ScopedAccessPermission>
          buffer_read_permission,
      media::mojom::VideoFrameInfoPtr frame_info) override;
  void OnBufferRetired(int buffer_id) override;
  void OnError() override;
  void OnLog(const std::string& message) override;
  void OnStarted() override;
  void OnStartedUsingGpuDecode() override;

  // content::VideoCaptureDeviceLauncher::Callbacks implementations
  void OnDeviceLaunched(
      std::unique_ptr<content::LaunchedVideoCaptureDevice> device) override;
  void OnDeviceLaunchFailed() override;
  void OnDeviceLaunchAborted() override;

 private:
  void OnFinishedConsumingBuffer(int buffer_context_id,
                                 double consumer_resource_utilization);

  const std::string device_id_;
  const content::MediaStreamType type_;
  const VideoCaptureParams params_;
  scoped_refptr<base::SingleThreadTaskRunner> device_task_runner_;
  mojom::VideoCaptureObserverPtr observer_;
  std::unique_ptr<content::VideoCaptureDeviceLauncher> device_launcher_;
  std::unique_ptr<content::LaunchedVideoCaptureDevice> launched_device_;

  // Error encounted. Capturing will be stopped immediately.
  bool encounted_error_ = false;

  // Unique ID assigned for the next buffer provided by OnNewBufferHandle().
  int next_buffer_context_id_ = 0;

  // Records the buffer context id for buffers that are not currently retired.
  // The |buffer_id| provided by OnNewBufferHandle() is used as the key.
  base::flat_map<int, int> id_map_;

  // Tracked the buffer_context_id for the retired buffers that are still held
  // by |observer_|.
  base::flat_set<int> retired_buffers_;

  // Records the |frame_feedback_id| and |buffer_read_permission| provided by
  // OnFrameReadyInBuffer(). The assigned buffer context id is used as the key.
  // Each buffer context is removed after it is returned from the |observer_|
  // by calling ReleaseBuffer(). When Stop() is called, all the buffer contexts
  // are cleared.
  using BufferContext = std::pair<
      int,
      std::unique_ptr<
          VideoCaptureDevice::Client::Buffer::ScopedAccessPermission>>;
  base::flat_map<int, BufferContext> buffer_context_map_;

  base::WeakPtrFactory<MirrorServiceVideoCaptureHost> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MirrorServiceVideoCaptureHost);
};

}  // namespace media

#endif  // CHROME_BROWSER_MEDIA_MIRROR_SERVICE_VIDEO_CAPTURE_HOST_H_
