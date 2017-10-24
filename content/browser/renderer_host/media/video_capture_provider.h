// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_BUILDABLE_VIDEO_CAPTURE_DEVICE_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_BUILDABLE_VIDEO_CAPTURE_DEVICE_H_

#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "content/public/common/media_stream_request.h"
#include "media/capture/video/video_capture_device.h"
#include "media/capture/video/video_capture_device_info.h"
#include "media/capture/video/video_frame_receiver.h"
#include "media/capture/video_capture_types.h"

namespace content {

class LaunchedVideoCaptureDevice;

// Asynchronously launches video capture devices. After a call to
// LaunchDeviceAsync() it is illegal to call LaunchDeviceAsync() again until
// |callbacks| has been notified about the outcome of the asynchronous launch.
class CONTENT_EXPORT VideoCaptureDeviceLauncher {
 public:
  class CONTENT_EXPORT Callbacks {
   public:
    virtual ~Callbacks() {}
    virtual void OnDeviceLaunched(
        std::unique_ptr<LaunchedVideoCaptureDevice> device) = 0;
    virtual void OnDeviceLaunchFailed() = 0;
    virtual void OnDeviceLaunchAborted() = 0;
  };

  virtual ~VideoCaptureDeviceLauncher() {}

  // The passed-in |done_cb| must guarantee that the context relevant
  // during the asynchronous processing stays alive.
  virtual void LaunchDeviceAsync(
      const std::string& device_id,
      MediaStreamType stream_type,
      const media::VideoCaptureParams& params,
      base::WeakPtr<media::VideoFrameReceiver> receiver,
      base::OnceClosure connection_lost_cb,
      Callbacks* callbacks,
      base::OnceClosure done_cb) = 0;

  virtual void AbortLaunch() = 0;
};

class LaunchedVideoCaptureDevice
    : public media::VideoFrameConsumerFeedbackObserver {
 public:
  // Device operation methods.
  virtual void GetPhotoState(
      media::VideoCaptureDevice::GetPhotoStateCallback callback) const = 0;
  virtual void SetPhotoOptions(
      media::mojom::PhotoSettingsPtr settings,
      media::VideoCaptureDevice::SetPhotoOptionsCallback callback) = 0;
  virtual void TakePhoto(
      media::VideoCaptureDevice::TakePhotoCallback callback) = 0;
  virtual void MaybeSuspendDevice() = 0;
  virtual void ResumeDevice() = 0;
  virtual void RequestRefreshFrame() = 0;

  // Methods for specific types of devices.
  virtual void SetDesktopCaptureWindowIdAsync(gfx::NativeViewId window_id,
                                              base::OnceClosure done_cb) = 0;
};

// Note: GetDeviceInfosAsync is only relevant for devices with
// MediaStreamType == MEDIA_DEVICE_VIDEO_CAPTURE, i.e. camera devices.
class CONTENT_EXPORT SingleClientVideoCaptureProvider {
 public:
  using GetDeviceInfosCallback = base::OnceCallback<void(
      const std::vector<media::VideoCaptureDeviceInfo>&)>;

  virtual ~SingleClientVideoCaptureProvider() {}

  // The passed-in |result_callback| must guarantee that the called
  // instance stays alive until |result_callback| is invoked.
  virtual void GetDeviceInfosAsync(GetDeviceInfosCallback result_callback) = 0;

  virtual std::unique_ptr<VideoCaptureDeviceLauncher>
  CreateDeviceLauncher() = 0;
};

class CONTENT_EXPORT VideoCaptureSession {
  virtual void Suspend() = 0;
  virtual void Resume() = 0;
  virtual void RequestRefreshFrame() = 0;
  // Closes the session. As soon as |done_cb| has been invoked, it is guaranteed
  // that no more frames are sent to the corresponding VideoFrameReceiver. Note:
  // Discarding the VideoCaptureSession instance also closes the session, but
  // does not produce an event indicating when it is safe to assume that no more
  // frames arrive at the VideoFrameReceiver.
  virtual void CloseAsync(base::OnceClosure done_cb) = 0;
};

// Note: GetDeviceInfosAsync is only relevant for devices with
// MediaStreamType == MEDIA_DEVICE_VIDEO_CAPTURE, i.e. camera devices.
class CONTENT_EXPORT MultiClientVideoCaptureProvider {
 public:
  struct ConnectToDeviceAnswer;

  using GetDeviceInfosCallback = base::OnceCallback<void(
      const std::vector<media::VideoCaptureDeviceInfo>&)>;
  using GetEligibleFormatsCallback =
      base::OnceCallback<void(const std::vector<media::VideoCaptureFormat>&)>;
  using ConnectToDeviceCallback =
      base::OnceCallback<void(ConnectToDeviceAnswer)>;

  enum ConnectToDeviceAnswerType {
    kDeviceNotFound,
    kConnectionEstablishedWithDesiredSettings,
    kConnectionEstablishedWithDifferentSettings,
  };

  struct ConnectToDeviceAnswer {
    ConnectToDeviceAnswerType answer_type;
    std::unique_ptr<VideoCaptureSession> session;
    media::VideoCaptureParams settings;
  };

  virtual ~MultiClientVideoCaptureProvider() {}

  // The passed-in |result_callback| must guarantee that the called
  // instance stays alive until |result_callback| is invoked.
  virtual void GetDeviceInfosAsync(GetDeviceInfosCallback result_callback) = 0;

  // Returns a list of formats that a newly connecting client is currently able
  // to obtain. In case that the device is already serving frames to another
  // client, the eligible formats may consist of just the single same format
  // that it is currently being served. If |device_id| does not correspond to
  // any known device, the returned |formats| is empty.
  virtual void GetEligibleFormatsAsync(
      const std::string& device_id,
      MediaStreamType stream_type,
      GetEligibleFormatsCallback result_callback) = 0;

  // Establishes a connection to a device, which may or may not be already in
  // use by different clients. Note that, since other clients may start using
  // a device at any time, it cannot be guaranteed that the connection will
  // deliver the |desired_settings|, even if they have been reported as eligible
  // in a previous call to GetEligibleFormats().
  // Clients need to check the |answer| to find out what settings they actually
  // obtained. Upon a successful connection, the provided |session| request will
  // be bound to a CaptureSession that is initially suspended. This allows
  // clients to abandon the session without receiving any frames in case they
  // received settings they cannot work with.
  // After the client closes (= releases) the |session| the |receiver| may still
  // receive frames, because of the asynchronous nature of the calls. If clients
  // require an event indicating that no more frames will be sent to |receiver|,
  // they are expected to call method Close() on the |session|.
  void ConnectToDeviceAsync(const std::string& device_id,
                            MediaStreamType stream_type,
                            const media::VideoCaptureParams& params,
                            base::WeakPtr<media::VideoFrameReceiver> receiver,
                            base::OnceClosure connection_lost_cb,
                            ConnectToDeviceCallback result_callback) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_BUILDABLE_VIDEO_CAPTURE_DEVICE_H_
