// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_PLATFORM_H_
#define CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_PLATFORM_H_

#include "base/callback_forward.h"
#include "build/build_config.h"

namespace chrome {
namespace platform {

void AttemptUserExit(base::OnceClosure attempt_user_exit_finish);
#if !defined(OS_ANDROID)
void AttemptRestartFinish();
#endif
void PreAttemptExit();
void PreAttemptRelaunch();

}  // namespace platform
}  // namespace chrome

#endif  // CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_PLATFORM_H_
