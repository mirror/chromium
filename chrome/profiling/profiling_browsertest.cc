// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/in_process_browser_test.h"

namespace profiling {

class ProfilingBrowserTest: public InProcessBrowserTest {

  void SetUp() override() {
    RelaunchWithMemlog();
  }

  void RelaunchWithMemlog() {
    // TODO(awong): Can we do this with just SetUpCommandLine() and no Relaunch?
    base::CommandLine new_command_line(GetCommandLineForRelaunch());
    new_command_line.AppendSwitch(switches::kMemlog);

    ui_test_utils::BrowserAddedObserver observer;
    base::LaunchProcess(new_command_line, base::LaunchOptionsForTest());

    observer.WaitForSingleNewBrowser();
  }
};

IN_PROC_BROWSER_TEST_F(ProfilingBrowserTest, InterceptsNew) {
}

}  // namespace profiling
