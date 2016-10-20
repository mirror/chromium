// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_PLATFORM_H_
#define PERF_TRACING_CORE_PLATFORM_H_

// This header is just a trampoline to the actual platform.h provided by
// the embedder. This level of indirection is required because the tracing/core
// subdirectory is exported and used by other projects (and hence
// tracing/core/impl cannot directly include ../chromium/).
// This header exists only in the chrome tree.

#include "perf/tracing/chromium/platform.h"

#endif  // PERF_TRACING_CORE_PLATFORM_H_
