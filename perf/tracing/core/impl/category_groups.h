// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_IMPL_CATEGORY_GROUPS_H_
#define PERF_TRACING_CORE_IMPL_CATEGORY_GROUPS_H_

#include "perf/tracing/core/common/macros.h"
#include "perf/tracing/core/impl/tracing_core_export.h"


namespace tracing {
namespace internal {

const unsigned char* TRACING_CORE_EXPORT GetCategoryGroupEnabled(const char* category_group);

}  // namespace internal
}  // namespace tracing

#endif  // PERF_TRACING_CORE_IMPL_CATEGORY_GROUPS_H_
