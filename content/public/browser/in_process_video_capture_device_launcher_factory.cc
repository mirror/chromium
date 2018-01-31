// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/in_process_video_capture_device_launcher_factory.h"

#include "content/browser/renderer_host/media/in_process_video_capture_device_launcher.h"

namespace content {

InProcessVideoCaptureDeviceLauncherFactory::
    InProcessVideoCaptureDeviceLauncherFactory(
        scoped_refptr<base::SingleThreadTaskRunner> device_task_runner)
    : device_task_runner_(device_task_runner) {}

InProcessVideoCaptureDeviceLauncherFactory::
    ~InProcessVideoCaptureDeviceLauncherFactory() {}

std::unique_ptr<VideoCaptureDeviceLauncher>
InProcessVideoCaptureDeviceLauncherFactory::CreateDeviceLauncher() {
  return std::make_unique<InProcessVideoCaptureDeviceLauncher>(
      device_task_runner_, nullptr);
}

}  // namespace content
