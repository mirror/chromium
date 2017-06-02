// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/download/service/download_background_task.h"

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
void StartBackgroundTask(JNIEnv* env,
                         const base::android::JavaParamRef<jobject>& jcaller,
                         const base::android::JavaParamRef<jobject>& jprofile,
                         jboolean is_download_task) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(jprofile);
  DCHECK(profile);

  DownloadService* download_service =
      DownloadServiceFactory::GetForBrowserContext(profile);
  DCHECK(download_service);

  // TODO(shaktisahu): Call scheduler or controller wake-up routine.
  // download_service->OnScheduledTaskStart(is_download_task);
}

DownloadBackgroundTask::DownloadBackgroundTask() = default;

DownloadBackgroundTask::~DownloadBackgroundTask() = default;

}  // namespace android
}  // namespace download
