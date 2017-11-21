// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/capture/fake_desktop_capture_device.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"

namespace content {

FakeDesktopCaptureDevice::FakeDesktopCaptureDevice(
    const DesktopMediaID& source) {
  DVLOG(2) << "FakeDesktopCaptureDevice for " << source.type;
}

FakeDesktopCaptureDevice::~FakeDesktopCaptureDevice() {
  DVLOG(2) << "FakeDesktopCaptureDevice@" << this << " destroying.";
}

// static
std::unique_ptr<media::VideoCaptureDevice> FakeDesktopCaptureDevice::Create(
    const DesktopMediaID& source) {
  return std::unique_ptr<media::VideoCaptureDevice>(
      new FakeDesktopCaptureDevice(source));
}

void FakeDesktopCaptureDevice::AllocateAndStart(
    const media::VideoCaptureParams& params,
    std::unique_ptr<Client> client) {
  DVLOG(1) << "Allocating " << params.requested_format.frame_size.ToString();
  client->OnStarted();
}

void FakeDesktopCaptureDevice::RequestRefreshFrame() {}

void FakeDesktopCaptureDevice::StopAndDeAllocate() {}

void FakeDesktopCaptureDevice::OnUtilizationReport(int frame_feedback_id,
                                                   double utilization) {}

}  // namespace content