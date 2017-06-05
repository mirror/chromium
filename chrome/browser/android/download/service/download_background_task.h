// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DOWNLOAD_SERVICE_DOWNLOAD_BACKGROUND_TASK_H_
#define CHROME_BROWSER_ANDROID_DOWNLOAD_SERVICE_DOWNLOAD_BACKGROUND_TASK_H_

#include <jni.h>
#include <memory>

#include "base/android/jni_android.h"
#include "base/macros.h"

namespace download {
namespace android {

using base::android::JavaParamRef;

// Entry point for servicing the task when the task is fired by the scheduler.
class DownloadBackgroundTask {
 public:
  // JNI registration.
  static bool RegisterDownloadBackgroundTask(JNIEnv* env);

  DownloadBackgroundTask();
  ~DownloadBackgroundTask();

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadBackgroundTask);
};

}  // namespace android
}  // namespace download

#endif  // CHROME_BROWSER_ANDROID_DOWNLOAD_SERVICE_DOWNLOAD_BACKGROUND_TASK_H_
