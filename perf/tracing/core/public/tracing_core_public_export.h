// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_API_TRACING_CORE_API_EXPORT_H_
#define PERF_TRACING_CORE_API_TRACING_CORE_API_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(TRACING_CORE_PUBLIC_IMPLEMENTATION)
#define TRACING_CORE_PUBLIC_EXPORT __declspec(dllexport)
#else
#define TRACING_CORE_PUBLIC_EXPORT __declspec(dllimport)
#endif  // defined(TRACING_CORE_PUBLIC_IMPLEMENTATION)

#else  // defined(WIN32)
#if defined(TRACING_CORE_PUBLIC_IMPLEMENTATION)
#define TRACING_CORE_PUBLIC_EXPORT __attribute__((visibility("default")))
#else
#define TRACING_CORE_PUBLIC_EXPORT
#endif  // defined(TRACING_CORE_PUBLIC_IMPLEMENTATION)
#endif

#else  // defined(COMPONENT_BUILD)
#define TRACING_CORE_PUBLIC_EXPORT
#endif

#endif  // PERF_TRACING_CORE_API_TRACING_CORE_API_EXPORT_H_
