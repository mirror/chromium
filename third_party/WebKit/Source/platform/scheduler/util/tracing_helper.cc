// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/util/tracing_helper.h"

namespace blink {
namespace scheduler {
namespace scheduler_tracing {

const char kCategoryNameDefault[] = "renderer.scheduler";
const char kCategoryNameInfo[] =
    TRACE_DISABLED_BY_DEFAULT("renderer.scheduler");
const char kCategoryNameDebug[] =
    TRACE_DISABLED_BY_DEFAULT("renderer.scheduler.debug");

namespace {

// No trace events should be created with this category.
const char kCategoryNameVerboseSnapshots[] =
    TRACE_DISABLED_BY_DEFAULT("renderer.scheduler.enable_verbose_snapshots");

}  // namespace

bool AreVerboseSnapshotsEnabled() {
  bool result = false;
  TRACE_EVENT_CATEGORY_GROUP_ENABLED(kCategoryNameVerboseSnapshots, &result);
  return result;
}

void WarmupCategories() {
  TRACE_EVENT_WARMUP_CATEGORY(kCategoryNameDefault);
  TRACE_EVENT_WARMUP_CATEGORY(kCategoryNameInfo);
  TRACE_EVENT_WARMUP_CATEGORY(kCategoryNameDebug);
  TRACE_EVENT_WARMUP_CATEGORY(kCategoryNameVerboseSnapshots);
}

}  // namespace scheduler_tracing
}  // namespace scheduler
}  // namespace blink
