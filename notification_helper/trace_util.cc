// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include <stdarg.h>
#include <stdio.h>
#include <string>

namespace notification_helper {

void Trace(const wchar_t* format_string, ...) {
  constexpr int kMaxLogBufferSize = 1024;
  wchar_t buffer[kMaxLogBufferSize] = {};

  va_list args = {};

  va_start(args, format_string);
  if (vswprintf(buffer, kMaxLogBufferSize, format_string, args) > 0) {
    OutputDebugString(buffer);
  } else {
    std::wstring error_string(L"Format error for string: ");
    OutputDebugString(error_string.append(format_string).c_str());
  }
  va_end(args);
}

}  // namespace notification_helper
