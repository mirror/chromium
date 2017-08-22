// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TEST_MULTIPROCESS_TEST_ANDROID_H_
#define BASE_TEST_MULTIPROCESS_TEST_ANDROID_H_

#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/process/process.h"

namespace base {

class CommandLine;
struct LaunchOptions;

// Similar to base::SpawnMultiProcessTestChild but also provide a
// MultiprocessTestClientLauncher.Delegate to the MultiprocessTestClientLauncher
// created.
Process SpawnMultiProcessTestChildWithDelegate(
    const std::string& procname,
    const CommandLine& base_command_line,
    const LaunchOptions& options,
    const android::ScopedJavaLocalRef<jobject> delegate);

}  // namespace base

#endif  // BASE_TEST_MULTIPROCESS_TEST_ANDROID_H_