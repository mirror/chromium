// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_ANDROID_PREFETCH_BACKGROUND_TASK_H_
#define CHROME_BROWSER_OFFLINE_PAGES_ANDROID_PREFETCH_BACKGROUND_TASK_H_

#include "base/android/jni_android.h"
#include "chrome/browser/offline_pages/android/prefetch_background_task_handler_impl.h"
#include "components/offline_pages/core/prefetch/prefetch_background_task_handler.h"
#include "components/offline_pages/core/prefetch/prefetch_dispatcher.h"
#include "components/offline_pages/core/prefetch/prefetch_service.h"
#include "net/base/backoff_entry.h"

class PrefRegistrySimple;

namespace offline_pages {

// A task with a counterpart in Java for managing the background activity of the
// offline page prefetcher.  Schedules and listens for events about prefetching
// tasks.
class PrefetchBackgroundTask : public PrefetchDispatcher::ScopedBackgroundTask {
 public:
  PrefetchBackgroundTask(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& j_prefetch_background_task,
      PrefetchService* service);
  ~PrefetchBackgroundTask() override;

  // API for interacting with BackgroundTaskScheduler from native.
  // Schedules the default 'NWake' prefetching task.
  // |additional_delay_seconds| is relative to the default 15 minute delay.
  static void Schedule(int additional_delay_seconds, bool update_current);

  // Cancels the default 'NWake' prefetching task.
  static void Cancel();

  // Java hooks.
  bool OnStopTask(JNIEnv* env,
                  const base::android::JavaParamRef<jobject>& jcaller);
  void SetTaskReschedulingForTesting(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcaller,
      jboolean reschedule,
      jboolean backoff);
  void SignalTaskFinishedForTesting(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& jcaller);

  // When this task completes, we tell the system whether the task should be
  // rescheduled with or without backoff.
  void SetNeedsReschedule(bool reschedule, bool backoff) override;
  bool needs_reschedule() { return needs_reschedule_; }

 private:
  PrefetchBackgroundTaskHandlerImpl* GetHandlerImpl();

  bool task_killed_by_system_ = false;
  bool needs_reschedule_ = false;
  bool needs_backoff_ = false;

  // A pointer to the controlling |PrefetchBackgroundTask|.
  base::android::ScopedJavaGlobalRef<jobject> java_prefetch_background_task_;

  // The PrefetchService owns |this|, so a raw pointer is OK.
  PrefetchService* service_;
};

void RegisterPrefetchBackgroundTaskPrefs(PrefRegistrySimple* registry);

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_ANDROID_PREFETCH_BACKGROUND_TASK_H_
