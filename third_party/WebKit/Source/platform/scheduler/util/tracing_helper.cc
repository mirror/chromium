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

const char kTracerNameUseCase[] = "RendererScheduler.UseCase";
const char kTracerNameBackgrounding[] = "RendererScheduler.Backgrounded";
const char kTracerNameAudioPlaying[] = "RendererScheduler.AudioPlaying";

namespace {

// No trace events should be created with this category.
const char kCategoryNameVerboseSnapshots[] =
    TRACE_DISABLED_BY_DEFAULT("renderer.scheduler.enable_verbose_snapshots");

// It was a desperate decision to implement it that way.
// TRACE_EVENT_* macros define static variable to cache a pointer to the state
// of category. There is no guarantee those macros will not introduce some
// other static variables. If they do it's likely to get a silent bug here.
// Hence, we need distinct versions of such variables for each(category, name)
// pair in order to pressume that we don't make unintended leak of state.
// For sake of clarity this template isn't exposed in the header to keep all
// instanciations in this translation unit.
template <const char* category, const char* name>
class StateTracerImpl : public StateTracer {
 public:
  StateTracerImpl(const void* object) : started_(false), object_(object) {}
  virtual ~StateTracerImpl() {
    if (started_)
      TRACE_EVENT_ASYNC_END0(category, name, object_);
  }

  void SetState(const char* state) override {
    if (started_)
      TRACE_EVENT_ASYNC_END0(category, name, object_);

    started_ = is_enabled();
    if (started_) {
      // Trace viewer logic relies on subslice starting at the exact same time
      // as the async event.
      base::TimeTicks now = base::TimeTicks::Now();
      TRACE_EVENT_ASYNC_BEGIN_WITH_TIMESTAMP0(category, name, object_, now);
      TRACE_EVENT_ASYNC_STEP_INTO_WITH_TIMESTAMP0(category, name, object_,
                                                  state, now);
    }
  }

 private:
  bool is_enabled() {
    bool result = false;
    TRACE_EVENT_CATEGORY_GROUP_ENABLED(category, &result);  // Cached.
    return result;
  }

  // We have to track |started_| state to avoid race condition since SetState
  // might be called before OnTraceLogEnabled notification.
  bool started_;
  const void* const object_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(StateTracerImpl);
};

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

// static
std::unique_ptr<StateTracer> StateTracers::CreateUseCase(const void* object) {
  return std::make_unique<
      StateTracerImpl<kCategoryNameDefault, kTracerNameUseCase>>(object);
}

// static
std::unique_ptr<StateTracer> StateTracers::CreateBackgrounding(
    const void* object) {
  return std::make_unique<
      StateTracerImpl<kCategoryNameDefault, kTracerNameBackgrounding>>(object);
}

// static
std::unique_ptr<StateTracer> StateTracers::CreateAudioPlaying(
    const void* object) {
  return std::make_unique<
      StateTracerImpl<kCategoryNameDefault, kTracerNameAudioPlaying>>(object);
}

}  // namespace scheduler_tracing
}  // namespace scheduler
}  // namespace blink
