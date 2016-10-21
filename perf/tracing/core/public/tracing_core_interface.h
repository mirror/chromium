// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_PUBLIC_TRACING_CORE_INTERFACE_H_
#define PERF_TRACING_CORE_PUBLIC_TRACING_CORE_INTERFACE_H_

#include "perf/tracing/core/public/tracing_core_public_export.h"

#include <stdint.h>

namespace tracing {
namespace internal {

// This is the interface for the dependency inversion between core and public.
// At any point there will be one and only one instance of core/impl.
class TRACING_CORE_PUBLIC_EXPORT TracingCoreInterface {
 public:
  static void SetInstance(TracingCoreInterface* inst) { instance_ = inst; }
  static TracingCoreInterface* instance() { return instance_; }

  virtual ~TracingCoreInterface() {};

  virtual uint64_t GetWallTime() = 0;
  virtual uint64_t GetThreadTime() = 0;
  const unsigned char* GetCategoryGroupEnabled(const char* category_group) = 0;
  virtual TraceEventHandle AddTraceEvent(/*todo args*/) = 0;

 private:
  TracingCoreInterface* instance_;
};

}  // namespace internal
}  // namespace tracing

#endif  // PERF_TRACING_CORE_PUBLIC_TRACING_CORE_INTERFACE_H_
