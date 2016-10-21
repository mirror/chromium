// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_PUBLIC_TRACE_EVENT_HANDLE_H_
#define PERF_TRACING_CORE_PUBLIC_TRACE_EVENT_HANDLE_H_

#include <stdint.h>

namespace tracing {

struct TraceEventHandle {
  uint32_t chunk_seq;

  // These numbers of bits must be kept consistent with
  // TraceBufferChunk::kMaxTrunkIndex and
  // TraceBufferChunk::kTraceBufferChunkSize (in trace_buffer.h).
  unsigned chunk_index : 26;
  unsigned event_index : 6;
};

}  // namespace tracing

#endif  // PERF_TRACING_CORE_PUBLIC_TRACE_EVENT_HANDLE_H_
