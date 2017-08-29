// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/android/locale_utils.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/strings/string_split.h"
#include "jni/LocaleUtils_jni.h"

namespace base {
namespace android {

std::string GetDefaultCountryCode() {
  JNIEnv* env = base::android::AttachCurrentThread();
  return ConvertJavaStringToUTF8(Java_LocaleUtils_getDefaultCountryCode(env));
}

std::string GetDefaultLocaleString() {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jstring> locale =
      Java_LocaleUtils_getDefaultLocaleString(env);
  return ConvertJavaStringToUTF8(locale);
}

std::vector<std::string> GetPreferredLocaleList() {
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jstring> localeList =
      Java_LocaleUtils_getDefaultLocaleListString(env);
  return base::SplitString(ConvertJavaStringToUTF8(localeList), ",",
                           base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
}

}  // namespace android
}  // namespace base
