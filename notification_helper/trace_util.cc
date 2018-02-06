// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include <stdarg.h>
#include <stdio.h>

#include "base/strings/string16.h"

namespace notification_helper {

#if !defined(NDEBUG)
// This is for developers only; we don't use this in circumstances
// (like release builds) where users could see it.
void Trace(const wchar_t* format_string, ...) {
  constexpr int kMaxLogBufferSize = 1024;
  wchar_t buffer[kMaxLogBufferSize] = {};

  va_list args = {};

  va_start(args, format_string);
  if (vswprintf(buffer, kMaxLogBufferSize, format_string, args) > 0) {
    OutputDebugString(buffer);
  } else {
    base::string16 error_string(L"Format error for string: ");
    OutputDebugString(error_string.append(format_string).c_str());
  }
  va_end(args);
}
#else
void Trace(const wchar_t* format_string, ...) {}
#endif  // !defined(NDEBUG)

}  // namespace notification_helper
