// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/android/jni_string.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/statistics_recorder.h"
#include "base/sys_info.h"
#include "jni/StatisticsRecorderAndroid_jni.h"

using base::android::JavaParamRef;
using base::android::ConvertUTF8ToJavaString;

namespace base {
namespace android {

static ScopedJavaLocalRef<jstring> ToJson(JNIEnv* env,
                                          const JavaParamRef<jclass>& clazz) {
  // Use lossy dump on low memory devices to reduce memory pressure, and
  // out-of-memory crashes in the browser process.
  base::JSONVerbosityLevel verbosity_level = base::JSONVerbosityLevel::kFull;
  if (base::SysInfo::IsLowEndDevice()) {
    verbosity_level = base::JSONVerbosityLevel::kSkipBuckets;
  }
  return ConvertUTF8ToJavaString(
      env, base::StatisticsRecorder::ToJSON(verbosity_level));
}

}  // namespace android
}  // namespace base
