// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/util/tracing_helper.h"

#include "base/trace_event/trace_event.h"

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
  base::trace_event::TraceLog::GetCategoryGroupEnabled(kCategoryNameDefault);
  base::trace_event::TraceLog::GetCategoryGroupEnabled(kCategoryNameInfo);
  base::trace_event::TraceLog::GetCategoryGroupEnabled(kCategoryNameDebug);
  base::trace_event::TraceLog::GetCategoryGroupEnabled(
      kCategoryNameVerboseSnapshots);
}

StateTracer::StateTracer(const char* category, const char* name, void* object)
    : category_(category),
      name_(name),
      object_(object),
      category_enabled_(TRACE_EVENT_API_GET_CATEGORY_GROUP_ENABLED(category)),
      started_(false) {
  DCHECK(category == kCategoryNameDefault || category == kCategoryNameInfo ||
         category == kCategoryNameDebug);
}

StateTracer::~StateTracer() {
  if (started_)
    TRACE_EVENT_ASYNC_END0(category_, name_, object_);
}

void StateTracer::SetState(const char* state) {
  if (started_)
    TRACE_EVENT_ASYNC_END0(category_, name_, object_);

  started_ = is_enabled();
  if (started_) {
    // Trace viewer logic relies on subslice starting at the exact same time
    // as the async event.
    base::TimeTicks now = base::TimeTicks::Now();
    TRACE_EVENT_ASYNC_BEGIN_WITH_TIMESTAMP0(category_, name_, object_, now);
    TRACE_EVENT_ASYNC_STEP_INTO_WITH_TIMESTAMP0(category_, name_, object_,
                                                state, now);
  }
}

}  // namespace scheduler_tracing
}  // namespace scheduler
}  // namespace blink
