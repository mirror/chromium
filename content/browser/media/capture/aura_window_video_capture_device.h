// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CAPTURE_AURA_WINDOW_VIDEO_CAPTURE_DEVICE_H_
#define CONTENT_BROWSER_MEDIA_CAPTURE_AURA_WINDOW_VIDEO_CAPTURE_DEVICE_H_

#include <memory>

#include "base/logging.h"
#include "base/macros.h"
#include "content/browser/media/capture/frame_sink_video_capture_device.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/desktop_media_id.h"

namespace content {

// Captures the displayed contents of an aura::Window, producing a stream of
// video frames.
class CONTENT_EXPORT AuraWindowVideoCaptureDevice
    : public FrameSinkVideoCaptureDevice {
 public:
  explicit AuraWindowVideoCaptureDevice(const DesktopMediaID& source);

  ~AuraWindowVideoCaptureDevice() override;

  // Creates a VideoCaptureDevice for the Aura desktop.  If |source| does not
  // reference a registered aura window, returns nullptr instead.
  static std::unique_ptr<AuraWindowVideoCaptureDevice> Create(
      const DesktopMediaID& source);

 private:
  class Targeter;

  // A helper that runs on the UI thread to monitor the target aura::Window, and
  // post a notification if it is destroyed.
  const std::unique_ptr<Targeter, BrowserThread::DeleteOnUIThread> targeter_;

  DISALLOW_COPY_AND_ASSIGN(AuraWindowVideoCaptureDevice);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CAPTURE_AURA_WINDOW_VIDEO_CAPTURE_DEVICE_H_
