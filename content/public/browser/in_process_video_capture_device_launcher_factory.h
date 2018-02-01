// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_IN_PROCESS_VIDEO_CAPTURE_DEVICE_LAUNCHER_FACTORY_H_
#define CONTENT_PUBLIC_BROWSER_IN_PROCESS_VIDEO_CAPTURE_DEVICE_LAUNCHER_FACTORY_H_

#include "base/single_thread_task_runner.h"
#include "content/public/browser/video_capture_device_launcher_factory.h"

namespace content {

class InProcessVideoCaptureDeviceLauncherFactory final
    : public VideoCaptureDeviceLauncherFactory {
 public:
  explicit InProcessVideoCaptureDeviceLauncherFactory(
      scoped_refptr<base::SingleThreadTaskRunner> device_task_runner);
  ~InProcessVideoCaptureDeviceLauncherFactory() override;

  // VideoCaptureDeviceLauncherFactory implementation
  std::unique_ptr<VideoCaptureDeviceLauncher> CreateDeviceLauncher() override;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> device_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(InProcessVideoCaptureDeviceLauncherFactory);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_IN_PROCESS_VIDEO_CAPTURE_DEVICE_LAUNCHER_FACTORY_H_
