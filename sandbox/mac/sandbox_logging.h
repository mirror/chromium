// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SANDBOX_SANDBOX_LOGGING_H_
#define SANDBOX_SANDBOX_LOGGING_H_

#define ABORT()                                                                \
  {                                                                            \
    asm volatile(                                                              \
        "int3; ud2; push %0;" ::"i"(static_cast<unsigned char>(__COUNTER__))); \
    __builtin_unreachable();                                                   \
  }

enum class LogLevel { FATAL, ERR, INFO, WARN };

namespace SandboxLogging {

void log(LogLevel level, const char* fmt, ...);

}  // namespace SandboxLogging

#endif  // SANDBOX_SANDBOX_LOGGING_H_
