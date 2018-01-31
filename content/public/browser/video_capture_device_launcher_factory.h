// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_VIDEO_CAPTURE_DEVICE_LAUNCHER_FACTORY_H_
#define CONTENT_PUBLIC_BROWSER_VIDEO_CAPTURE_DEVICE_LAUNCHER_FACTORY_H_

#include "content/public/browser/video_capture_device_launcher.h"

namespace content {

class VideoCaptureDeviceLauncherFactory {
 public:
  virtual ~VideoCaptureDeviceLauncherFactory() {}
  virtual std::unique_ptr<VideoCaptureDeviceLauncher>
  CreateDeviceLauncher() = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_VIDEO_CAPTURE_DEVICE_LAUNCHER_FACTORY_H_
