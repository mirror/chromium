// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/launcher/test_launcher.h"
#include "chrome/test/base/chrome_test_launcher.h"
#include "chrome/test/base/chrome_test_suite.h"

int main(int argc, char** argv) {
  size_t parallel_jobs = base::NumParallelJobs() / 2U;
  if (parallel_jobs == 0U) {
    return 1;
  }

  ChromeTestSuiteRunner runner;
  ChromeTestLauncherDelegate delegate(&runner);
  return LaunchChromeTests(parallel_jobs, &delegate, argc, argv);
}
