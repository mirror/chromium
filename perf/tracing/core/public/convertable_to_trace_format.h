// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_PUBLIC_CONVERTABLE_TO_TRACE_FORMAT_H_
#define PERF_TRACING_CORE_PUBLIC_CONVERTABLE_TO_TRACE_FORMAT_H_

#include <string>

#include "perf/tracing/core/common/macros.h"
#include "perf/tracing/core/public/tracing_core_public_export.h"

namespace tracing {

class TraceEventMemoryOverhead;

// For any argument of type TRACE_VALUE_TYPE_CONVERTABLE the provided
// class must implement this interface.
class TRACING_CORE_PUBLIC_EXPORT ConvertableToTraceFormat {
 public:
  ConvertableToTraceFormat() {}
  virtual ~ConvertableToTraceFormat() {}

  // Append the class info to the provided |out| string. The appended
  // data must be a valid JSON object. Strings must be properly quoted, and
  // escaped. There is no processing applied to the content after it is
  // appended.
  virtual void AppendAsTraceFormat(std::string* out) const = 0;
  virtual void EstimateTraceMemoryOverhead(TraceEventMemoryOverhead*);

  std::string ToString() const;

 private:
  TRACE_DISALLOW_COPY_AND_ASSIGN(ConvertableToTraceFormat);
};

}  // namespace tracing

#endif  // PERF_TRACING_CORE_PUBLIC_CONVERTABLE_TO_TRACE_FORMAT_H_
