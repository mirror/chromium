// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/browser/offline_pages/android/prefetch_background_task_android.h"

#include <memory>

#include "base/logging.h"
#include "base/time/time.h"
#include "chrome/browser/offline_pages/prefetch/prefetch_background_task.h"
#include "chrome/browser/offline_pages/prefetch/prefetch_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "components/offline_pages/core/offline_page_feature.h"
#include "components/offline_pages/core/prefetch/prefetch_service.h"
#include "jni/PrefetchBackgroundTask_jni.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;

namespace offline_pages {

namespace prefetch {

// JNI call to start request processing in scheduled mode.
static jboolean StartPrefetchTask(JNIEnv* env,
                                  const JavaParamRef<jobject>& jcaller,
                                  const JavaParamRef<jobject>& jprofile) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(jprofile);
  DCHECK(profile);

  PrefetchService* prefetch_service =
      PrefetchServiceFactory::GetForBrowserContext(profile);
  if (!prefetch_service)
    return false;

  prefetch_service->GetPrefetchDispatcher()->BeginBackgroundTask(
      base::MakeUnique<PrefetchBackgroundTaskAndroid>(env, jcaller,
                                                      prefetch_service));
  return true;
}

}  // namespace prefetch

// static
void PrefetchBackgroundTask::Schedule(int additional_delay_seconds,
                                      bool update_current) {
  JNIEnv* env = base::android::AttachCurrentThread();
  return prefetch::Java_PrefetchBackgroundTask_scheduleTask(
      env, additional_delay_seconds, update_current);
}

// static
void PrefetchBackgroundTask::Cancel() {
  JNIEnv* env = base::android::AttachCurrentThread();
  return prefetch::Java_PrefetchBackgroundTask_cancelTask(env);
}

PrefetchBackgroundTaskAndroid::PrefetchBackgroundTaskAndroid(
    JNIEnv* env,
    const JavaParamRef<jobject>& java_prefetch_background_task,
    PrefetchService* service)
    : java_prefetch_background_task_(java_prefetch_background_task),
      service_(service) {
  // Give the Java side a pointer to the new background task object.
  prefetch::Java_PrefetchBackgroundTask_setNativeTask(
      env, java_prefetch_background_task_, reinterpret_cast<jlong>(this));
}

PrefetchBackgroundTaskAndroid::~PrefetchBackgroundTaskAndroid() {
  JNIEnv* env = base::android::AttachCurrentThread();
  prefetch::Java_PrefetchBackgroundTask_doneProcessing(
      env, java_prefetch_background_task_, NeedsReschedule());

  PrefetchBackgroundTaskHandler* handler =
      service_->GetPrefetchBackgroundTaskHandler();
  if (reschedule_type_ ==
      PrefetchBackgroundTaskRescheduleType::RETRY_WITH_BACKOFF) {
    handler->Backoff();
  } else {
    handler->ResetBackoff();
  }

  if (NeedsReschedule()) {
    PrefetchBackgroundTask::Schedule(GetAdditionalSeconds(),
                                     true /*update_current*/);
  }
}

bool PrefetchBackgroundTaskAndroid::OnStopTask(
    JNIEnv* env,
    const JavaParamRef<jobject>& jcaller) {
  task_killed_by_system_ = true;
  service_->GetPrefetchDispatcher()->StopBackgroundTask();
  return false;
}

void PrefetchBackgroundTaskAndroid::SetTaskReschedulingForTesting(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    int reschedule_type) {
  SetReschedule(
      static_cast<PrefetchBackgroundTaskRescheduleType>(reschedule_type));
}

void PrefetchBackgroundTaskAndroid::SignalTaskFinishedForTesting(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  service_->GetPrefetchDispatcher()->RequestFinishBackgroundTaskForTest();
}

void PrefetchBackgroundTaskAndroid::SetReschedule(
    PrefetchBackgroundTaskRescheduleType type) {
  switch (type) {
    // |SUSPEND| takes highest precendence.
    case PrefetchBackgroundTaskRescheduleType::SUSPEND:
      reschedule_type_ = PrefetchBackgroundTaskRescheduleType::SUSPEND;
      break;
    // |RETRY_WITH_BACKOFF| takes 2nd highest precendence and thus it can't
    // overwrite |SUSPEND|.
    case PrefetchBackgroundTaskRescheduleType::RETRY_WITH_BACKOFF:
      if (reschedule_type_ != PrefetchBackgroundTaskRescheduleType::SUSPEND) {
        reschedule_type_ =
            PrefetchBackgroundTaskRescheduleType::RETRY_WITH_BACKOFF;
      }
      break;
    // |RETRY_WITHOUT_BACKOFF| takes 3rd highest precendence and thus it only
    // overwrites the lowest precendence |NO_RETRY|.
    case PrefetchBackgroundTaskRescheduleType::RETRY_WITHOUT_BACKOFF:
      if (reschedule_type_ == PrefetchBackgroundTaskRescheduleType::NO_RETRY) {
        reschedule_type_ =
            PrefetchBackgroundTaskRescheduleType::RETRY_WITHOUT_BACKOFF;
      }
      break;
    case PrefetchBackgroundTaskRescheduleType::NO_RETRY:
      break;
    default:
      NOTREACHED();
      break;
  }
}

int PrefetchBackgroundTaskAndroid::GetAdditionalSeconds() const {
  // The task should be rescheduled after 1 day if the suspension is requested.
  // This will happen even when the task is then killed due to the system.
  if (reschedule_type_ == PrefetchBackgroundTaskRescheduleType::SUSPEND)
    return static_cast<int>(base::TimeDelta::FromDays(1).InSeconds());

  // If the task is killed due to the system, it should be rescheduled without
  // backoff even when it is in effect because we want to rerun the task asap.
  if (task_killed_by_system_)
    return 0;

  return service_->GetPrefetchBackgroundTaskHandler()
      ->GetAdditionalBackoffSeconds();
}

bool PrefetchBackgroundTaskAndroid::NeedsReschedule() const {
  return reschedule_type_ != PrefetchBackgroundTaskRescheduleType::NO_RETRY ||
         task_killed_by_system_;
}

}  // namespace offline_pages
