// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "perf/tracing/core/public/tracing_core_public_export.h"

#include "perf/tracing/platform_light.h"

namespace tracing {
namespace internal {

TracingCore::TracingCore() {}
TracingCore::~TracingCore() {}

uint64_t TracingCore::GetWallTime() {
  return ::tracing::platform::GetWallTime();
}

uint64_t TracingCore::GetThreadTime() {
  return ::tracing::platform::GetThreadTime();
}

TraceEventHandle TracingCore::AddEvent(/*todo args*/) {
  // TraceLog...->Add...
}

}  // namespace internal
}  // namespace tracing
