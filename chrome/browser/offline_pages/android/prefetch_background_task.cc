// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/browser/offline_pages/android/prefetch_background_task.h"

#include <memory>

#include "base/time/time.h"
#include "chrome/browser/offline_pages/prefetch/prefetch_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "chrome/common/pref_names.h"
#include "components/offline_pages/core/offline_page_feature.h"
#include "components/offline_pages/core/prefetch/prefetch_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
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
      base::MakeUnique<PrefetchBackgroundTask>(env, jcaller, prefetch_service));
  return true;
}

}  // namespace prefetch

void RegisterPrefetchBackgroundTaskPrefs(PrefRegistrySimple* registry) {
  registry->RegisterListPref(prefs::kOfflinePrefetchBackoff);
}

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

PrefetchBackgroundTask::PrefetchBackgroundTask(
    JNIEnv* env,
    const JavaParamRef<jobject>& java_prefetch_background_task,
    PrefetchService* service)
    : java_prefetch_background_task_(java_prefetch_background_task),
      service_(service) {
  // Give the Java side a pointer to the new background task object.
  prefetch::Java_PrefetchBackgroundTask_setNativeTask(
      env, java_prefetch_background_task_, reinterpret_cast<jlong>(this));
}

PrefetchBackgroundTask::~PrefetchBackgroundTask() {
  JNIEnv* env = base::android::AttachCurrentThread();
  prefetch::Java_PrefetchBackgroundTask_doneProcessing(
      env, java_prefetch_background_task_.obj(), needs_reschedule_);

  if (needs_backoff_)
    GetHandlerImpl()->Backoff();

  if (needs_reschedule_) {
    // If the task is killed due to the system, it should be rescheduled without
    // backoff even when it is in effect because we want to rerun the task asap.
    Schedule(task_killed_by_system_
                 ? 0
                 : GetHandlerImpl()->GetAdditionalBackoffSeconds(),
             true /*update_current*/);
  }
}

bool PrefetchBackgroundTask::OnStopTask(JNIEnv* env,
                                        const JavaParamRef<jobject>& jcaller) {
  task_killed_by_system_ = true;
  needs_reschedule_ = true;
  service_->GetPrefetchDispatcher()->StopBackgroundTask();
  return false;
}

void PrefetchBackgroundTask::SetTaskReschedulingForTesting(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    jboolean reschedule,
    jboolean backoff) {
  SetNeedsReschedule(static_cast<bool>(reschedule), static_cast<bool>(backoff));
}

void PrefetchBackgroundTask::SignalTaskFinishedForTesting(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  service_->GetPrefetchDispatcher()->RequestFinishBackgroundTaskForTest();
}

void PrefetchBackgroundTask::SetNeedsReschedule(bool reschedule, bool backoff) {
  needs_reschedule_ = needs_reschedule_ || reschedule;
  needs_backoff_ = needs_backoff_ || backoff;
}

PrefetchBackgroundTaskHandlerImpl* PrefetchBackgroundTask::GetHandlerImpl() {
  return static_cast<PrefetchBackgroundTaskHandlerImpl*>(
      service_->GetPrefetchBackgroundTaskHandler());
}

}  // namespace offline_pages
