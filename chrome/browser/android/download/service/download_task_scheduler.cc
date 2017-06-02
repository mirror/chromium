// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/download/service/download_task_scheduler.h"

#include "base/android/jni_android.h"
#include "jni/DownloadTaskScheduler_jni.h"

namespace download {
namespace android {

DownloadTaskScheduler::DownloadTaskScheduler() = default;

DownloadTaskScheduler::~DownloadTaskScheduler() = default;

void DownloadTaskScheduler::ScheduleDownloadTask(
    bool require_charging,
    bool require_unmetered_network) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_DownloadTaskScheduler_scheduleDownloadTask(env, require_charging,
                                                  require_unmetered_network);
}

void DownloadTaskScheduler::ScheduleCleanupTask(long keep_alive_time_ms) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_DownloadTaskScheduler_scheduleCleanupTask(env, keep_alive_time_ms);
}

}  // namespace android
}  // namespace download
