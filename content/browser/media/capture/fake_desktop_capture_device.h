// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CAPTURE_FAKE_DESKTOP_CAPTURE_DEVICE_H_
#define CONTENT_BROWSER_MEDIA_CAPTURE_FAKE_DESKTOP_CAPTURE_DEVICE_H_

#include "content/browser/media/capture/desktop_capture_device.h"
#include "content/public/browser/desktop_media_id.h"
#include "ui/gfx/native_widget_types.h"

namespace content {

// An implementation of VideoCaptureDevice that fakes a desktop capturer.
class CONTENT_EXPORT FakeDesktopCaptureDevice : public DesktopCaptureDevice {
 public:
  static std::unique_ptr<media::VideoCaptureDevice> Create(
      const DesktopMediaID& source);

  ~FakeDesktopCaptureDevice() override;

  // VideoCaptureDevice implementation.
  void AllocateAndStart(const media::VideoCaptureParams& params,
                        std::unique_ptr<Client> client) override;
  void RequestRefreshFrame() override;
  void StopAndDeAllocate() override;
  void OnUtilizationReport(int frame_feedback_id, double utilization) override;

  // Set the platform-dependent window id for the notification window.
  void SetNotificationWindowId(gfx::NativeViewId window_id) override;

 private:
  explicit FakeDesktopCaptureDevice(const DesktopMediaID& source);

  DISALLOW_COPY_AND_ASSIGN(FakeDesktopCaptureDevice);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CAPTURE_DESKTOP_FAKE_CAPTURE_DEVICE_H_