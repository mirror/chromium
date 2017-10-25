// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FIRST_RUN_FIRST_RUN_DIALOG_H_
#define CHROME_BROWSER_FIRST_RUN_FIRST_RUN_DIALOG_H_

#include "build/build_config.h"

// Hide this function on platforms where the dialog does not exist.
#if defined(OS_MACOSX) || (defined(OS_LINUX) && !defined(OS_CHROMEOS))

class Profile;

namespace first_run {

// Shows the first run dialog. Only called for organic first runs on Mac and
// desktop Linux official builds when metrics reporting is not already enabled.
// Invokes ChangeMetricsReportingState() if consent is given to enable crash
// reporting, and may initiate the flow to set the default browser.
void ShowFirstRunDialog(Profile* profile);

// A function pointer invoked before calling ShowFirstRunDialog() if non null.
extern void (*g_before_show_first_run_dialog_hook_for_testing)();

}  // namespace first_run

#endif  // OS_MACOSX || DESKTOP_LINUX

#endif  // CHROME_BROWSER_FIRST_RUN_FIRST_RUN_DIALOG_H_
