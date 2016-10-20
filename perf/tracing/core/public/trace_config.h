// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_PUBLIC_TRACE_CONFIG_H_
#define PERF_TRACING_CORE_PUBLIC_TRACE_CONFIG_H_

#include <string>

#include "perf/tracing/core/common/macros.h"
#include "perf/tracing/core/public/tracing_core_public_export.h"

namespace tracing {

class TRACING_CORE_PUBLIC_EXPORT TraceConfig {
 public:
   void InitializeDefault();

   // Initialize from category filter and trace options strings
   void InitializeFromStrings(StringPiece category_filter_string,
                              StringPiece trace_options_string);

   bool IsCategoryEnabled(const char*);
   bool IsCategoryEnabled(const std::string&);

   const std::unordered_set<std::string>& included_categories() const;
   const std::unordered_set<std::string>& excluded_categories() const;

   virtual void ToJSON(stf::string* out) const;

 private:
  TRACE_DISALLOW_COPY_AND_ASSIGN(TraceConfig);
};

}  // namespace tracing

#endif  // PERF_TRACING_CORE_PUBLIC_TRACE_CONFIG_H_
