// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_TRACING_HELPER_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_TRACING_HELPER_H_

#include "base/macros.h"

// TODO(kraynov): Move platform/scheduler/base/trace_helper.h here.

namespace blink {
namespace scheduler {
namespace scheduler_tracing {

extern const char kCategoryNameDefault[];
extern const char kCategoryNameInfo[];
extern const char kCategoryNameDebug[];

void WarmupCategories();
bool AreVerboseSnapshotsEnabled();

class StateTracer {
 public:
  // Category must be a constant listed in this header.
  StateTracer(const char* category, const char* name, void* object);
  ~StateTracer();

  void SetState(const char* state);

 private:
  // No pointers are owned by StateTracer instance.
  const char* const category_;
  const char* const name_;
  const void* const object_;
  const unsigned char* category_enabled_;

  bool is_enabled() { return *category_enabled_; }

  // We need to explicitly track |started_| state to avoid race condition
  // during RendererScheduler creation — it's created before receiving a
  // OnTraceLogEnabled notification.
  bool started_;

  DISALLOW_COPY_AND_ASSIGN(StateTracer);
};

}  // namespace scheduler_tracing
}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_TRACING_HELPER_H_
