// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_PLATFORM_FULL_H_
#define PERF_TRACING_CORE_PLATFORM_FULL_H_

// This header is just a trampoline to the actual platform_full.h provided by
// the embedder. This level of indirection is required because the tracing/core
// subdirectory is exported and used by other projects (and hence cannot
// directly include ../chromium/) while this header exists only in the chrome
// tree.

#include "perf/tracing/chromium/platform_full.h"

#endif  // PERF_TRACING_CORE_PLATFORM_FULL_H_
