// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_IMPL_CATEGORY_GROUPS_H_
#define PERF_TRACING_CORE_IMPL_CATEGORY_GROUPS_H_

namespace tracing {
namespace internal {

const unsigned char* GetCategoryGroupEnabled(const char* category_group);

}  // namespace internal
}  // namespace tracing

#endif  // PERF_TRACING_CORE_IMPL_CATEGORY_GROUPS_H_
