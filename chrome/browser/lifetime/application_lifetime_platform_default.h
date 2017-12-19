// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_PLATFORM_DEFAULT_H_
#define CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_PLATFORM_DEFAULT_H_

#include "base/callback_forward.h"

// application_lifetime_platform.h implementation for all OSes except Chrome OS
// and Android.
namespace chrome {
namespace platform {

void AttemptExit();
void AttemptRestartFinish();
void AttemptUserExit(base::OnceClosure attempt_user_exit_finish);
void PreAttemptRelaunch();

}  // namespace platform
}  // namespace chrome

#endif  // CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_PLATFORM_DEFAULT_H_
