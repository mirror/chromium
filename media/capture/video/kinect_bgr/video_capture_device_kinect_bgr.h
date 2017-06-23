// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_KINECT_BGR_H_
#define MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_KINECT_BGR_H_

#include "base/threading/thread_checker.h"
#include "media/capture/video/video_capture_device.h"
#include "media/capture/video_capture_types.h"
#include "third_party/kinect_bgr/kinect_device.h"

namespace media {

// Implementation of media::VideoCaptureDevice that receives its frames from
// a Kinect camera with background-removal applied via third_party/kinect_bgr.
class VideoCaptureDeviceKinectBgr : public VideoCaptureDevice {
 public:
  VideoCaptureDeviceKinectBgr();
  ~VideoCaptureDeviceKinectBgr() override;

  // VideoCaptureDevice implementation.
  void AllocateAndStart(
      const VideoCaptureParams& params,
      std::unique_ptr<VideoCaptureDevice::Client> client) override;
  void StopAndDeAllocate() override;
  void TakePhoto(TakePhotoCallback callback) override;
  void GetPhotoState(GetPhotoStateCallback callback) override;
  void SetPhotoOptions(mojom::PhotoSettingsPtr settings,
                       SetPhotoOptionsCallback callback) override;

 private:
  std::unique_ptr<VideoCaptureDevice::Client> client_;
  std::unique_ptr<kinectdevice::KinectDevice> device_;
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_WIN_VIDEO_CAPTURE_DEVICE_KINECT_BGR_H_
