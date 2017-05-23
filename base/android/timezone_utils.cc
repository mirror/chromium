// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/timezone_utils.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/strings/string16.h"
#include "jni/TimezoneUtils_jni.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"

namespace base {
namespace android {

icu::TimeZone* DetectHostTimeZone() {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::string16 tzid =
      ConvertJavaStringToUTF16(Java_TimezoneUtils_getDefaultTimeZoneId(env));
  return icu::TimeZone::createTimeZone(
      icu::UnicodeString(FALSE, tzid.data(), tzid.length()));
}

}  // namespace android
}  // namespace base
