// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_ANDROID_TIMEZONE_UTILS_H_
#define BASE_ANDROID_TIMEZONE_UTILS_H_

#include <jni.h>

#include "base/base_export.h"
#include "base/strings/string16.h"

namespace base {
namespace android {

// Return the current default timezone ID (Olson)
BASE_EXPORT base::string16 GetDefaultTimeZoneId();

}  // namespace android
}  // namespace base

#endif  // BASE_ANDROID_LOCALE_UTILS_H_
