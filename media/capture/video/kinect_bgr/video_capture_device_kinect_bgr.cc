// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/kinect_bgr/video_capture_device_kinect_bgr.h"

namespace media {

VideoCaptureDeviceKinectBgr::VideoCaptureDeviceKinectBgr() = default;

VideoCaptureDeviceKinectBgr::~VideoCaptureDeviceKinectBgr() = default;

void VideoCaptureDeviceKinectBgr::AllocateAndStart(
    const VideoCaptureParams& params,
    std::unique_ptr<VideoCaptureDevice::Client> client) {
  NOTIMPLEMENTED();
}

void VideoCaptureDeviceKinectBgr::StopAndDeAllocate() {
  NOTIMPLEMENTED();
}

void VideoCaptureDeviceKinectBgr::TakePhoto(TakePhotoCallback callback) {
  NOTIMPLEMENTED();
}

void VideoCaptureDeviceKinectBgr::GetPhotoState(
    GetPhotoStateCallback callback) {
  NOTIMPLEMENTED();
}

void VideoCaptureDeviceKinectBgr::SetPhotoOptions(
    mojom::PhotoSettingsPtr settings,
    VideoCaptureDevice::SetPhotoOptionsCallback callback) {
  NOTIMPLEMENTED();
}

}  // namespace media
