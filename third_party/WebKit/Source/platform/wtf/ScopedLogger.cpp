// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/wtf/ScopedLogger.h"

#include "build/build_config.h"
#include "platform/wtf/ThreadSpecific.h"

static void vprintf_stderr_common(const char* format, va_list args) {
#if defined(OS_MACOSX)
  va_list copyOfArgs;
  va_copy(copyOfArgs, args);
  asl_vlog(0, 0, ASL_LEVEL_NOTICE, format, copyOfArgs);
  va_end(copyOfArgs);
#elif defined(OS_ANDROID)
  __android_log_vprint(ANDROID_LOG_WARN, "WebKit", format, args);
#elif defined(OS_WIN)
  if (IsDebuggerPresent()) {
    size_t size = 1024;

    do {
      char* buffer = (char*)malloc(size);
      if (!buffer)
        break;

      if (_vsnprintf(buffer, size, format, args) != -1) {
        OutputDebugStringA(buffer);
        free(buffer);
        break;
      }

      free(buffer);
      size *= 2;
    } while (size > 1024);
  }
#endif
  vfprintf(stderr, format, args);
}

#if DCHECK_IS_ON()

namespace WTF {

ScopedLogger::ScopedLogger(bool condition, const char* format, ...)
    : parent_(condition ? Current() : 0), multiline_(false) {
  if (!condition)
    return;

  va_list args;
  va_start(args, format);
  Init(format, args);
  va_end(args);
}

ScopedLogger::~ScopedLogger() {
  if (Current() == this) {
    if (multiline_)
      Indent();
    else
      Print(" ");
    Print(")\n");
    Current() = parent_;
  }
}

void ScopedLogger::SetPrintFuncForTests(PrintFunctionPtr ptr) {
  print_func_ = ptr;
};

void ScopedLogger::Init(const char* format, va_list args) {
  Current() = this;
  if (parent_)
    parent_->WriteNewlineIfNeeded();
  Indent();
  Print("( ");
  print_func_(format, args);
}

void ScopedLogger::WriteNewlineIfNeeded() {
  if (!multiline_) {
    Print("\n");
    multiline_ = true;
  }
}

void ScopedLogger::Indent() {
  if (parent_) {
    parent_->Indent();
    PrintIndent();
  }
}

void ScopedLogger::Log(const char* format, ...) {
  if (Current() != this)
    return;

  va_list args;
  va_start(args, format);

  WriteNewlineIfNeeded();
  Indent();
  PrintIndent();
  print_func_(format, args);
  Print("\n");

  va_end(args);
}

void ScopedLogger::Print(const char* format, ...) {
  va_list args;
  va_start(args, format);
  print_func_(format, args);
  va_end(args);
}

void ScopedLogger::PrintIndent() {
  Print("  ");
}

ScopedLogger*& ScopedLogger::Current() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<ScopedLogger*>, ref, ());
  return *ref;
}

ScopedLogger::PrintFunctionPtr ScopedLogger::print_func_ =
    vprintf_stderr_common;

}  // namespace WTF

#endif  // DCHECK_IS_ON
