// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/captiveportal/portal_login_helper.h"

#include "jni/CaptivePortalLoginLauncher_jni.h"

namespace chrome {
namespace android {

void LaunchCaptive(JNIEnv* env) {
  Java_CaptivePortalLoginLauncher_launch(env);
}

}  // namespace android
}  // namespace chrome
