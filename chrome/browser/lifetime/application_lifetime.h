// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_H_
#define CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_H_

#include "base/compiler_specific.h"
#include "build/build_config.h"

class Browser;

namespace chrome {

// This is internal function to be called from per-platform AttemptExit
// implementations.
// Triggers notifications and g_browser_process->platform_part()->AttemptExit();
void AttemptExitImpl(bool try_to_quit_application);

// Starts a user initiated exit process. Called from Browser::Exit.
// On platforms other than ChromeOS, this is equivalent to
// CloseAllBrowsers() On ChromeOS, this tells session manager
// that chrome is signing out, which lets session manager send
// SIGTERM to start actual exit process.
void AttemptUserExit();

// Starts a user initiated restart process. On platforms other than
// chromeos, this sets a restart bit in the preference so that
// chrome will be restarted at the end of shutdown process. On
// ChromeOS, this simply exits the chrome, which lets sesssion
// manager re-launch the browser with restore last session flag.
void AttemptRestart();

// Starts a user initiated relaunch process. On platforms other than
// chromeos, this is equivalent to AttemptRestart. On ChromeOS, this relaunches
// the entire OS, instead of just relaunching the browser.
void AttemptRelaunch();

// Attempt to exit by closing all browsers.  This is equivalent to
// CloseAllBrowsers() on platforms where the application exits
// when no more windows are remaining. On other platforms (the Mac),
// this will additionally exit the application if all browsers are
// successfully closed.
//  Note that the exit process may be interrupted by download or
// unload handler, and the browser may or may not exit.
void AttemptExit();

#if !defined(OS_ANDROID)
// Returns true if all browsers can be closed without user interaction.
// This currently checks if there is pending download, or if it needs to
// handle unload handler.
bool AreAllBrowsersCloseable();

// Closes all browsers and if successful, quits.
void CloseAllBrowsersAndQuit();

// Closes all browsers. If the session is ending the windows are closed
// directly. Otherwise the windows are closed by way of posting a WM_CLOSE
// message. This will quit the application if there is nothing other than
// browser windows keeping it alive or the application is quitting.
void CloseAllBrowsers();

// Marks shutdown intented, not a crash.
void MarkAsCleanShutdown();

// If there are no browsers open and we aren't already shutting down,
// initiate a shutdown.
void ShutdownIfNeeded();

// Begins shutdown of the application when the desktop session is ending.
void SessionEnding();

// Called once the application is exiting.
void OnAppExiting();

// Called once the application is exiting to do any platform specific
// processing required.
void HandleAppExitingForPlatform();
#endif  // !defined(OS_ANDROID)

}  // namespace chrome

#endif  // CHROME_BROWSER_LIFETIME_APPLICATION_LIFETIME_H_
