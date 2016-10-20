// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_PUBLIC_TRACE_CONFIG_H_
#define PERF_TRACING_CORE_PUBLIC_TRACE_CONFIG_H_

#include <string>

#include "base/macros.h"
#include "perf/tracing/core/public/trace_config.h"

namespace tracing {

class TRACING_CHROMIUM_PUBLIC_EXPORT ChromiumTraceConfig : public TraceConfig {
 public:
   // TODO: Extra memory-infra stuff.
   // TODO: Synthetic delays

  void ToJSON(std::string* out) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromiumTraceConfig);
};

}  // namespace tracing

#endif  // PERF_TRACING_CORE_PUBLIC_TRACE_CONFIG_H_
