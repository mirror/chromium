// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/download/service/download_background_task.h"

#include "base/android/callback_android.h"
#include "base/callback_forward.h"
#include "chrome/browser/download/download_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "components/download/public/download_service.h"
#include "content/public/browser/browser_context.h"
#include "jni/DownloadBackgroundTask_jni.h"

namespace download {
namespace android {

// static
bool DownloadBackgroundTask::RegisterDownloadBackgroundTask(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

// static
void StartBackgroundTask(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    const base::android::JavaParamRef<jobject>& jprofile,
    jint task_type,
    const base::android::JavaParamRef<jobject>& jcallback) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(jprofile);
  DCHECK(profile);

  auto task_finished_callback =
      [](const base::android::JavaRef<jobject>& j_callback,
         bool needs_reschedule) {
        RunCallbackAndroid(j_callback, needs_reschedule);
      };

  TaskFinishedCallback finish_callback =
      base::Bind(task_finished_callback,
                 base::android::ScopedJavaGlobalRef<jobject>(jcallback));
  DownloadService* download_service =
      DownloadServiceFactory::GetForBrowserContext(profile);
  download_service->OnStartScheduledTask(
      static_cast<DownloadTaskType>(task_type), finish_callback);

  if (profile->HasOffTheRecordProfile()) {
    download_service = DownloadServiceFactory::GetForBrowserContext(
        profile->GetOffTheRecordProfile());
    download_service->OnStartScheduledTask(
        static_cast<DownloadTaskType>(task_type), finish_callback);
  }
}

// static
jboolean StopBackgroundTask(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    const base::android::JavaParamRef<jobject>& jprofile,
    jint task_type) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(jprofile);
  DCHECK(profile);

  DownloadService* download_service =
      DownloadServiceFactory::GetForBrowserContext(profile);
  bool needs_reschedule = download_service->OnStopScheduledTask(
      static_cast<DownloadTaskType>(task_type));

  if (profile->HasOffTheRecordProfile()) {
    download_service = DownloadServiceFactory::GetForBrowserContext(
        profile->GetOffTheRecordProfile());
    needs_reschedule |= download_service->OnStopScheduledTask(
        static_cast<DownloadTaskType>(task_type));
  }

  return needs_reschedule;
}

DownloadBackgroundTask::DownloadBackgroundTask() = default;

DownloadBackgroundTask::~DownloadBackgroundTask() = default;

}  // namespace android
}  // namespace download
