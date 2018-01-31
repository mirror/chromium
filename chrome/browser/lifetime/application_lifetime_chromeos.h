// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_CHROMEOS_H_
#define CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_CHROMEOS_H_

#include "base/callback_forward.h"

// Chrome OS - specific exit code.

// Starts restart flow.
void AttemptRestartChromeOS();

// Starts exit flow.
void PreAttemptUserExitChromeOS();

// Returns true if any of the chrome::platform::Attempt{UserExit,Relaunch,Exit}
// calls above have been called.
bool IsAttemptingShutdownChromeOS();

// Shutdown chrome cleanly without blocking. This is called
// when SIGTERM is received on Chrome OS, and always sets
// exit-cleanly bit and exits the browser, even if there is
// ongoing downloads or a page with onbeforeunload handler.
//
// If you need to exit or restart in your code on ChromeOS,
// use AttemptExit or AttemptRestart respectively.
void ExitCleanlyChromeOS();

#endif  // CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_CHROMEOS_H_
