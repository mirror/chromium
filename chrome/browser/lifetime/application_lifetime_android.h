// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_ANDROID_H_
#define CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_ANDROID_H_

#include "base/android/jni_android.h"
#include "base/callback_forward.h"

namespace chrome {

// application_lifetime_platform.h implementation.
namespace platform {
void AttemptUserExit(base::OnceClosure attempt_user_exit_finish);
void AttemptRestartFinish();
void PreAttemptExit();
void PreAttemptRelaunch();
}  // namespace platform

void TerminateAndroid();

}  // namespace chrome

#endif  // CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_ANDROID_H_
