// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_CAPTIVEPORTAL_PORTAL_LOGIN_HELPER_H_
#define CHROME_BROWSER_ANDROID_CAPTIVEPORTAL_PORTAL_LOGIN_HELPER_H_

#include <stddef.h>

#include <jni.h>

namespace chrome {
namespace android {

void LaunchCaptive(JNIEnv* env);

}  // namespace android
}  // namespace chrome

#endif  // CHROME_BROWSER_ANDROID_CAPTIVEPORTAL_PORTAL_LOGIN_HELPER_H_
