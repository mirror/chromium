// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_PLATFORM_H_
#define CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_PLATFORM_H_

#include "base/callback_forward.h"
#include "build/build_config.h"

namespace chrome {
namespace platform {

// This file declares platform-specific calls.
// Chrome OS and Android implementations are in specific files.
// Other OSes use implementations from application_lifetime_platform_default.cc

// See AttemptExit() in application_lifetime.h
void AttemptExit();

#if !defined(OS_ANDROID)
// This is called from AttemptRestart() declared in application_lifetime.h
void AttemptRestartFinish();
#endif

// This is called at the begining of AttemptUserExit declared in
// application_lifetime.h.
// |attempt_user_exit_finish| - platform-specific exit implementation, which
// must be executed at the end (ignored on Chrome OS).
void PreAttemptUserExit(base::OnceClosure attempt_user_exit_finish);

// This is called at the begining of AttemptRelaunch declared in
// application_lifetime.h.
void PreAttemptRelaunch();

}  // namespace platform
}  // namespace chrome

#endif  // CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_PLATFORM_H_
