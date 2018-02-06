// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NOTIFICATION_HELPER_TRACE_UTIL_H_
#define NOTIFICATION_HELPER_TRACE_UTIL_H_

#if defined(NDEBUG)
#define Trace(...) ((void)0)
#else
#define Trace(const wchar_t* format_string, ...) \
  TraceImpl(const wchar_t* format_string, __VA_ARGS__)
#endif  // defined(NDEBUG)

#endif  // NOTIFICATION_HELPER_TRACE_UTIL_H_
