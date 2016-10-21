// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_PUBLIC_TRACING_CORE_H_
#define PERF_TRACING_CORE_PUBLIC_TRACING_CORE_H_

#include "perf/tracing/core/impl/tracing_core_export.h"
#include "perf/tracing/core/public/tracing_core_interface.h"

namespace tracing {
namespace internal {

// The implementation of the TracingCoreInterface which is injected into the
// tracing/public layer. 
class TRACING_CORE_EXPORT TracingCore : public TracingCoreInterface {
 public:
  TracingCore();
  ~TracingCore() override;

  uint64_t GetWallTime() override;
  uint64_t GetThreadTime() override;
  TraceEventHandle AddTraceEvent(/*todo args*/) override;

 private:
  TracingCore();
};

}  // namespace internal
}  // namespace tracing


#endif  // PERF_TRACING_CORE_PUBLIC_TRACING_CORE_H_
