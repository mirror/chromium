// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/util/tracing_helper.h"

#include "base/format_macros.h"
#include "base/strings/stringprintf.h"

using base::trace_event::TraceLog;

namespace blink {
namespace scheduler {

const char kTracingCategoryNameDefault[] = "renderer.scheduler";
const char kTracingCategoryNameInfo[] =
    TRACE_DISABLED_BY_DEFAULT("renderer.scheduler");
const char kTracingCategoryNameDebug[] =
    TRACE_DISABLED_BY_DEFAULT("renderer.scheduler.debug");

namespace {

// No trace events should be created with this category.
const char kTracingCategoryNameVerboseSnapshots[] =
    TRACE_DISABLED_BY_DEFAULT("renderer.scheduler.enable_verbose_snapshots");

}  // namespace

namespace internal {

void ValidateTracingCategory(const char* category) {
  // Category must be a constant defined in tracing_helper.
  // Unfortunately, static_assert won't work with templates because
  // inequality (!=) of linker symbols is undefined in compile-time.
  DCHECK(category == kTracingCategoryNameDefault ||
         category == kTracingCategoryNameInfo ||
         category == kTracingCategoryNameDebug);
}

}  // namespace internal


template <const char* category>
class StateTracer {
 public:
  StateTracer(const char* name, const void* object)
      : category_flags(TraceLog::GetCategoryGroupEnabled(category)),
        name_(name),
        object_(object),
        started_(false) {
  }

  ~StateTracer() {
    if (started_)
      TRACE_EVENT_ASYNC_END0(category, name_, object_);
  }

  void Trace(const char* state) {
    if (started_)
      TRACE_EVENT_ASYNC_END0(category, name_, object_);

    started_ = internal::is_category_enabled(category_flags);
    if (started_) {
      // Trace viewer logic relies on subslice starting at the exact same time
      // as the async event.
      base::TimeTicks now = base::TimeTicks::Now();
      TRACE_EVENT_ASYNC_BEGIN_WITH_TIMESTAMP0(category, name_, object_, now);
      TRACE_EVENT_ASYNC_STEP_INTO_WITH_TIMESTAMP0(category, name_, object_,
                                                  state, now);
    }
  }

  const unsigned char* const category_flags;  // Not owned.

 private:
  const char* const name_;  // Not owned.
  const void* const object_;  // Not owned.
  // We have to track |started_| state to avoid race condition since SetState
  // might be called before OnTraceLogEnabled notification.
  bool started_;

  DISALLOW_COPY_AND_ASSIGN(StateTracer);
};


class RendererTracers {
 public:
  RendererTracers(const void* object)
      : audio_playing("RendererScheduler.AudioPlaying", object) {
  }

  StateTracer<kTracingCategoryNameDefault> audio_playing;
};

RendererTracing::RendererTracing(const void* object)
    : tracers_(new RendererTracers(object)) {
  registry_ = base::Bind(
      &RendererTracing::RegisterOnTraceLogEnabled, base::Unretained(this));
}

RendererTracing::~RendererTracing() {
  delete tracers_;
}

Glue<bool> RendererTracing::AudioPlaying() {
  return {
    registry_,
    tracers_->audio_playing.category_flags,
    base::Bind([](RendererTracers* tracers, bool value) {
      if (value)
        tracers->audio_playing.Trace("playing");
      else
        tracers->audio_playing.Trace("muted");
    }, base::Unretained(tracers_))
  };
}

void RendererTracing::OnTraceLogEnabled() {
  for (auto& callback : on_trace_log_enabled_) {
    callback.Run();
  }
}

void RendererTracing::RegisterOnTraceLogEnabled(
    base::Callback<void()> callback) {
  on_trace_log_enabled_.push_back(callback);
}


bool AreVerboseSnapshotsEnabled() {
  bool result = false;
  TRACE_EVENT_CATEGORY_GROUP_ENABLED(kTracingCategoryNameVerboseSnapshots,
                                     &result);
  return result;
}

void WarmupTracingCategories() {
  TRACE_EVENT_WARMUP_CATEGORY(kTracingCategoryNameDefault);
  TRACE_EVENT_WARMUP_CATEGORY(kTracingCategoryNameInfo);
  TRACE_EVENT_WARMUP_CATEGORY(kTracingCategoryNameDebug);
  TRACE_EVENT_WARMUP_CATEGORY(kTracingCategoryNameVerboseSnapshots);
}

namespace util {

std::string PointerToString(const void* pointer) {
  return base::StringPrintf(
      "0x%" PRIx64,
      static_cast<uint64_t>(reinterpret_cast<uintptr_t>(pointer)));
}

const char* BackgroundStateToString(bool is_backgrounded) {
  if (is_backgrounded) {
    return "backgrounded";
  } else {
    return "foregrounded";
  }
}

const char* YesNoStateToString(bool is_yes) {
  if (is_yes) {
    return "yes";
  } else {
    return "no";
  }
}

double TimeDeltaToMilliseconds(const base::TimeDelta& value) {
  return value.InMillisecondsF();
}

}  // namespace util

}  // namespace scheduler
}  // namespace blink
