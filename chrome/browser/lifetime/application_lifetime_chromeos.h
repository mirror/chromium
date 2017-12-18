// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_CHROMEOS_H_
#define CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_CHROMEOS_H_

#include "base/callback_forward.h"

// application_lifetime_platform.h implementation.
namespace chrome {
namespace platform {

void AttemptUserExit(base::OnceClosure attempt_user_exit_finish);
void AttemptRestartFinish();
void PreAttemptExit();
void PreAttemptRelaunch();

}  // namespace platform
}  // namespace chrome

// Chrome OS - specific exit code.
namespace chromeos {

// Starts restart flow.
void AttemptRestart();

// Starts exit flow.
void AttemptUserExit();

// Returns true if any of the chrome::platform::Attempt{UserExit,Relaunch,Exit}
// calls above have been called.
bool IsAttemptingShutdown();

// This is called from chrome::ExitCleanly() before exiting.
void PreExitCleanly();

}  // namespace chromeos

#endif  // CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_CHROMEOS_H_
