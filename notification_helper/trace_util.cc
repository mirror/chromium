// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include <stdarg.h>
#include <stdio.h>

namespace notification_helper {

void Trace(const wchar_t* format_string, ...) {
  static const int kMaxLogBufferSize = 1024;
  static wchar_t buffer[kMaxLogBufferSize] = {};

  va_list args = {};

  va_start(args, format_string);
  vswprintf(buffer, kMaxLogBufferSize, format_string, args);
  OutputDebugStringW(buffer);
  va_end(args);
}

}  // namespace notification_helper
