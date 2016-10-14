// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_COMMON_MACROS_H_
#define PERF_TRACING_CORE_COMMON_MACROS_H_

#define TRACING_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;      \
  void operator=(const TypeName&) = delete

// Macro for hinting that an expression is likely to be false.
#if !defined(TRACE_UNLIKELY)
#if defined(__GNUC__)
#define TRACING_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define TRACING_UNLIKELY(x) (x)
#endif  // defined(__GNUC__)
#endif  // !defined(TRACE_UNLIKELY)

#endif  // PERF_TRACING_CORE_COMMON_MACROS_H_
