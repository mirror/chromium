// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/util/state_tracing_helper.h"

#include "base/trace_event/trace_event.h"

namespace blink {
namespace scheduler {

StateTracingHelper::StateTracingHelper(const char* tracing_category,
                                       const char* name,
                                       void* object,
                                       const char* state)
    : tracing_category_(tracing_category), name_(name), object_(object) {}

StateTracingHelper::~StateTracingHelper() {
  TRACE_EVENT_ASYNC_END0(tracing_category_, name_, object_);
}

void StateTracingHelper::Start(const char* state) {
  TRACE_EVENT_ASYNC_BEGIN0(tracing_category_, name_, object_);
  TRACE_EVENT_ASYNC_STEP_INTO0(tracing_category_, name_, object_, state);
}

void StateTracingHelper::SetState(const char* state) {
  TRACE_EVENT_ASYNC_END0(tracing_category_, name_, object_);
  Start(state);
}

}  // namespace scheduler
}  // namespace blink
