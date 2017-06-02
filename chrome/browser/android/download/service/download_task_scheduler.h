// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DOWNLOAD_SERVICE_DOWNLOAD_TASK_SCHEDULER_H_
#define CHROME_BROWSER_ANDROID_DOWNLOAD_SERVICE_DOWNLOAD_TASK_SCHEDULER_H_

#include <jni.h>
#include <memory>

#include "base/macros.h"

namespace download {
namespace android {

// TODO(shaktisahu) : Remove this interface definition after merging
// with xingliu@'s CL.
class PlatformTaskScheduler {
 public:
  virtual void ScheduleDownloadTask(bool require_charging,
                                    bool require_unmetered_network) = 0;
  virtual void ScheduleCleanupTask(long keep_alive_time_ms) = 0;
};

// DownloadTaskScheduler is the utility class to schedule various background
// tasks with the OS as of when required by the download service.
class DownloadTaskScheduler : public PlatformTaskScheduler {
 public:
  DownloadTaskScheduler();
  ~DownloadTaskScheduler();

  void ScheduleDownloadTask(bool require_charging,
                            bool require_unmetered_network) override;
  void ScheduleCleanupTask(long keep_alive_time_ms) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadTaskScheduler);
};

}  // namespace android
}  // namespace download

#endif  // CHROME_BROWSER_ANDROID_DOWNLOAD_SERVICE_DOWNLOAD_TASK_SCHEDULER_H_
