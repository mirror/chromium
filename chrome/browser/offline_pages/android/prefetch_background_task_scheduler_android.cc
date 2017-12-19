// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/prefetch/prefetch_background_task_scheduler.h"

#include <memory>

#include "jni/PrefetchBackgroundTaskScheduler_jni.h"

namespace offline_pages {

// static
void PrefetchBackgroundTaskScheduler::Schedule(int additional_delay_seconds,
                                               bool limitlessPrefetching) {
  JNIEnv* env = base::android::AttachCurrentThread();
  prefetch::Java_PrefetchBackgroundTaskScheduler_scheduleTask(
      env, additional_delay_seconds, limitlessPrefetching);
}

// static
void PrefetchBackgroundTaskScheduler::Cancel() {
  JNIEnv* env = base::android::AttachCurrentThread();
  prefetch::Java_PrefetchBackgroundTaskScheduler_cancelTask(env);
}

}  // namespace offline_pages
