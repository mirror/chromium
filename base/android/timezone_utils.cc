// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/timezone_utils.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "jni/TimezoneUtils_jni.h"

namespace base {
namespace android {

base::string16 GetDefaultTimeZoneId() {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jstring> timezone =
      Java_TimezoneUtils_getDefaultTimeZoneId(env);
  return ConvertJavaStringToUTF16(timezone);
}

}  // namespace android
}  // namespace base
