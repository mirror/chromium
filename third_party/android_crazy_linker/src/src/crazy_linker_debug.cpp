// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crazy_linker_debug.h"

#include <errno.h>
#include <string.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

namespace crazy {

#if CRAZY_DEBUG

namespace {

// Helper class to model a buffer with an AppendFormat() and Output()
// methods.
struct Buffer {
  static const int kBufferSize = 4096;

  int pos = 0;
  char buffer[kBufferSize];

  void AppendFormat(const char* fmt, const char* str) {
    int ret = snprintf(buffer, kBufferSize - pos, fmt, str);
    pos += ret;
    if (pos > kBufferSize)
      pos = kBufferSize - 1;
  }

  void AppendFormat(const char* fmt, va_list args) {
    int ret = vsnprintf(buffer, kBufferSize - pos, fmt, args);
    pos += ret;
    if (pos > kBufferSize)
      pos = kBufferSize - 1;
  }

  void Output() {
    // First, send to stderr.
    fprintf(stderr, "%.*s\n", pos, buffer);

#ifdef __ANDROID__
    // Then to the Android log.
    __android_log_write(ANDROID_LOG_INFO, "crazy_linker", buffer);
#endif
  }
};

}  // namespace

void Log(const char* location, const char* fmt, ...) {
  int old_errno = errno;
  Buffer buf;
  va_list args;
  va_start(args, fmt);
  buf.AppendFormat("%s: ", location);
  buf.AppendFormat(fmt, args);
  buf.Output();
  va_end(args);
  errno = old_errno;
}

void LogErrno(const char* location, const char* fmt, ...) {
  int old_errno = errno;
  Buffer buf;
  va_list args;
  va_start(args, fmt);
  buf.AppendFormat("%s: ", location);
  buf.AppendFormat(fmt, args);
  buf.AppendFormat(": %s", strerror(old_errno));
  buf.Output();
  va_end(args);
  errno = old_errno;
}

#endif  // CRAZY_DEBUG

}  // namespace crazy
