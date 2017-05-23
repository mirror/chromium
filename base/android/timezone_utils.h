// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ANDROID_TIMEZONE_UTILS_H_
#define BASE_ANDROID_TIMEZONE_UTILS_H_

#include <jni.h>

#include "base/base_export.h"
#include "base/strings/string16.h"
#include "third_party/icu/source/common/unicode/uversion.h"

// 'icu' does not work. Use U_ICU_NAMESPACE.
namespace U_ICU_NAMESPACE {

class TimeZone;

}  // namespace U_ICU_NAMESPACE

namespace base {
namespace android {

// Return an ICU timezone created from the host timezone.
BASE_EXPORT icu::TimeZone* DetectHostTimeZone();

}  // namespace android
}  // namespace base

#endif  // BASE_ANDROID_TIMEZONE_UTILS_H_
