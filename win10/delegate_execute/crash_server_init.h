// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WIN10_DELEGATE_EXECUTE_CRASH_SERVER_INIT_H_
#define WIN10_DELEGATE_EXECUTE_CRASH_SERVER_INIT_H_

#include <memory>

namespace google_breakpad {
class ExceptionHandler;
}

namespace delegate_execute {

// Initializes breakpad crash reporting and returns a pointer to a newly
// constructed ExceptionHandler object. It is the responsibility of the caller
// to delete this object which will shut down the crash reporting machinery.
std::unique_ptr<google_breakpad::ExceptionHandler> InitializeCrashReporting();

}  // namespace delegate_execute

#endif  // WIN10_DELEGATE_EXECUTE_CRASH_SERVER_INIT_H_
