// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "perf/tracing/core/public/convertable_to_trace_format.h"

namespace tracing {

void ConvertableToTraceFormat::EstimateTraceMemoryOverhead(
    TraceEventMemoryOverhead* overhead) {
  // TODO here.
}

std::string ConvertableToTraceFormat::ToString() const {
  std::string result;
  AppendAsTraceFormat(&result);
  return result;
}

}  // namespace tracing
