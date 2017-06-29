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
    : i420_buffer_size_in_bytes_(0), weak_ptr_factory_(this) {}

VideoCaptureDeviceKinectBgr::~VideoCaptureDeviceKinectBgr() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

// static
void VideoCaptureDeviceKinectBgr::GetSupportedFormats(
    VideoCaptureFormats* formats) {
  formats->push_back(
      VideoCaptureFormat(gfx::Size(1920, 1080), 30.0f, PIXEL_FORMAT_I420));
}

void VideoCaptureDeviceKinectBgr::AllocateAndStart(
    const VideoCaptureParams& params,
    std::unique_ptr<VideoCaptureDevice::Client> client) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  first_ref_time_ = base::TimeTicks();
  device_ = base::MakeUnique<kinectdevice::KinectDevice>(
      std::bind(&OnKinectDeviceCallback, weak_ptr_factory_.GetWeakPtr(),
                base::SequencedTaskRunnerHandle::Get(), std::placeholders::_1));
  kinectdevice::Configurations config;
  config.playback = true;
  config.process_status_log = true;
  config.playback_path = "C:\\TestDataSet\\playback15\\";
  config.face_model_path =
      "C:\\TestDataSet\\adshaarcascade_frontalface_default.xml";
  device_->SetConfig(config);
  bool success = device_->Start();
  if (!success) {
    client->OnError(FROM_HERE, "device_->Start() returned false");
    return;
  }
  client_ = std::move(client);
  client_->OnStarted();
}

void VideoCaptureDeviceKinectBgr::StopAndDeAllocate() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  device_->Stop();
  weak_ptr_factory_.InvalidateWeakPtrs();
  i420_buffer_size_in_bytes_ = 0;
  i420_buffer_.reset();
}

void VideoCaptureDeviceKinectBgr::TakePhoto(TakePhotoCallback callback) {
  NOTIMPLEMENTED();
}

void VideoCaptureDeviceKinectBgr::GetPhotoState(
    GetPhotoStateCallback callback) {}

void VideoCaptureDeviceKinectBgr::SetPhotoOptions(
    mojom::PhotoSettingsPtr settings,
    VideoCaptureDevice::SetPhotoOptionsCallback callback) {
  NOTIMPLEMENTED();
}

void VideoCaptureDeviceKinectBgr::OnFrameData(
    std::unique_ptr<kinectdevice::FrameData> frame_data) {
  const gfx::Size dimensions(frame_data->width, frame_data->height);
  const int rgb_data_size_in_bytes = frame_data->width * frame_data->height * 3;
  const int i420_data_size_in_bytes = static_cast<int>(
      VideoFrame::AllocationSize(PIXEL_FORMAT_I420, dimensions));
  const int alpha_data_size_in_bytes = frame_data->width * frame_data->height;
  const float kArbitraryFrameRate = 30.0f;
  const int kFrameFeedbackId = static_cast<int>(frame_data->frame_index);
  base::TimeTicks now = base::TimeTicks::Now();
  if (first_ref_time_.is_null())
    first_ref_time_ = now;
  client_->OnIncomingCapturedData(
      frame_data->BGR_buf, rgb_data_size_in_bytes,
      VideoCaptureFormat(gfx::Size(frame_data->width, frame_data->height),
                         kArbitraryFrameRate, PIXEL_FORMAT_RGB24),
      0, now, now - first_ref_time_, kFrameFeedbackId);

  if (!i420_buffer_ || i420_buffer_size_in_bytes_ < i420_data_size_in_bytes) {
    i420_buffer_ = std::make_unique<uint8_t[]>(i420_data_size_in_bytes);
    memset(i420_buffer_.get(), 0, i420_data_size_in_bytes);
    i420_buffer_size_in_bytes_ = i420_data_size_in_bytes;
  }
  // Copy alpha data to the y-channel.
  memcpy(i420_buffer_.get(), frame_data->mask_buf, alpha_data_size_in_bytes);

  client_->OnIncomingCapturedData(
      i420_buffer_.get(), i420_data_size_in_bytes,
      VideoCaptureFormat(dimensions, kArbitraryFrameRate, PIXEL_FORMAT_I420), 0,
      now, now - first_ref_time_, kFrameFeedbackId);
}

}  // namespace media
