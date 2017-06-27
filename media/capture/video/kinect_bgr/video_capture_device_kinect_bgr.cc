// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/kinect_bgr/video_capture_device_kinect_bgr.h"

#include "base/sequenced_task_runner.h"

namespace media {

namespace {

void OnKinectDeviceCallback(
    base::WeakPtr<VideoCaptureDeviceKinectBgr> device,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    std::unique_ptr<kinectdevice::FrameData> frame_data) {
  task_runner->PostTask(
      FROM_HERE, base::Bind(
                     [](base::WeakPtr<VideoCaptureDeviceKinectBgr> device,
                        std::unique_ptr<kinectdevice::FrameData> frame_data) {
                       if (!device)
                         return;
                       device->OnFrameData(std::move(frame_data));
                     },
                     device, base::Passed(&frame_data)));
}

}  // anonymous namespace

VideoCaptureDeviceKinectBgr::VideoCaptureDeviceKinectBgr()
    : weak_ptr_factory_(this) {}

VideoCaptureDeviceKinectBgr::~VideoCaptureDeviceKinectBgr() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

// static
void VideoCaptureDeviceKinectBgr::GetSupportedFormats(
    VideoCaptureFormats* formats) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  formats->push_back(
      VideoCaptureFormat(gfx::Size(1280, 720), 30.0f, PIXEL_FORMAT_I420));
}

void VideoCaptureDeviceKinectBgr::AllocateAndStart(
    const VideoCaptureParams& params,
    std::unique_ptr<VideoCaptureDevice::Client> client) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  device_ = base::MakeUnique<kinectdevice::KinectDevice>(
      std::bind(&OnKinectDeviceCallback, weak_ptr_factory_.GetWeakPtr(),
                base::SequencedTaskRunnerHandle::Get(), std::placeholders::_1));
  kinectdevice::Configurations config;
  config.playback = true;
  config.process_status_log = true;
  config.playback_path = "C:\\TestDataSet\\playback15\\";
  config.face_model_path =
      "C:\\TestDataSet\\haarcascade_frontalface_default.xml";
  device_->SetConfig(config);
  bool success = device_->Start();
  if (!success) {
    client->OnError(FROM_HERE, "device_->Start() returned false");
    return;
  }
  client_ = std::move(client);
}

void VideoCaptureDeviceKinectBgr::StopAndDeAllocate() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  device_->Stop();
  weak_ptr_factory_.InvalidateWeakPtrs();
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

void VideoCaptureDeviceKinectBgr::OnFrameData(
    std::unique_ptr<kinectdevice::FrameData> frame_data) {
  LOG(ERROR) << "Frame received";
}

}  // namespace media
