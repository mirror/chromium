// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROFILING_HOST_PROFILING_TEST_DRIVER_H_
#define CHROME_BROWSER_PROFILING_HOST_PROFILING_TEST_DRIVER_H_

#include "chrome/browser/profiling_host/profiling_process_host.h"

namespace profiling {

// This class runs tests for the profiling service, a cross-platform,
// multi-process component. Chrome on Android does not support browser_tests. It
// does support content_browsertests, but those are not multi-process tests. On
// Android, processes have to be started via the Activity mechanism, and the
// test infrastructure does not support this.
//
// To avoid test-code duplication, all tests are pulled into this class.
// browser_tests will directly call this class. The android
// chrome_public_test_apk will invoke this class via a JNI shim. Since the
// latter is not running within the gtest framework, this class cannot use
// EXPECT* and ASSERT* macros. Instead, this class will return a bool indicating
// success of the entire test. On failure, errors will be output via LOG(ERROR).
// These will show up in the browser_tests output stream, and will be captured
// by logcat [the Android logging facility]. The latter is already the canonical
// mechanism for investigating test failures.
//
// Note: Outputting to stderr will not have the desired effect, since that is
// not captured by logcat.
class ProfilingTestDriver {
public:
  struct Options {
    // The profiling mode to test.
    ProfilingProcessHost::Mode mode;

    // Whether the caller has already started profiling with the given mode.
    bool profiling_already_started;
  };

  ProfilingTestDriver();
  ~ProfilingTestDriver();

  // Must be called on the content::BrowserThread::UI thread.
  // Returns whether the test run was successful. Expectation/Assertion failures
  // will be printed via LOG(ERROR).
  bool RunTest(const Options& options);

 private:
  // If profiling is expected to already be started, confirm it.
  // Otherwise, start profiling with the given mode.
  bool CheckOrStartProfiling();

  Options options_;
};

}  // namespace profiling

#endif  // CHROME_BROWSER_PROFILING_HOST_PROFILING_TEST_DRIVER_H_
