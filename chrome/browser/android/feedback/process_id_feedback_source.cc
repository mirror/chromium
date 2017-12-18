// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/process/process_handle.h"
#include "jni/ProcessIdFeedbackSource_jni.h"

using base::android::JavaParamRef;

namespace chrome {
namespace android {

int64_t JNI_ProcessIdFeedbackSource_GetCurrentPid(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
  return base::GetCurrentProcId();
}

}  // namespace android
}  // namespace chrome
