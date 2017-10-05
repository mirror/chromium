// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_SCHEDULER_TRACING_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_SCHEDULER_TRACING_H_

#include <functional>
#include "platform/PlatformExport.h"
#include "platform/scheduler/renderer/renderer_scheduler_types.h"
#include "platform/scheduler/util/tracing_helper.h"
#include "v8/include/v8.h"

namespace blink {
namespace scheduler {

PLATFORM_EXPORT extern
    const char* AudioPlayingStateToString(bool is_audio_playing);
PLATFORM_EXPORT extern
    const char* BackgroundedStateToString(bool is_backgrounded);
PLATFORM_EXPORT extern const char* RAILModeToString(v8::RAILMode rail_mode);
PLATFORM_EXPORT extern const char* UseCaseToString(UseCase use_case);
PLATFORM_EXPORT extern const char* YesNoStateToString(bool state);

namespace internal {

template <typename ValueType, class Tracer>
class TraceableValue {
 public:
  void Set(ValueType new_value) {
    if (new_value != value_) {
      value_ = new_value;
      TraceNow();
    }
  }

  ValueType Get() const {
    return value_;
  }
  operator ValueType() const {
    return Get();
  }

  void TraceNow() {
    if (tracer_.is_enabled())
      tracer_.SetState(converter_(value_));
  }

 protected:
  TraceableValue(const char* name,
                 ValueType initial_value,
                 const void* owner,
                 std::function<const char*(ValueType)> converter)
      : value_(initial_value), tracer_(name, owner), converter_(converter) {}

 private:
  ValueType value_;
  Tracer tracer_;
  std::function<const char*(ValueType)> converter_;

  DISALLOW_COPY_AND_ASSIGN(TraceableValue);
};

}  // namespace internal

class AudioPlayingValue : public internal::TraceableValue<bool,
                          StateTracer<kTracingCategoryNameDefault>> {
 public:
  AudioPlayingValue(bool initial_value, const void* owner)
      : TraceableValue("RendererScheduler.AudioPlaying",
                       initial_value, owner, AudioPlayingStateToString) {}
};

class TouchstartExpectedSoonValue : public internal::TraceableValue<bool,
                                    StateTracer<kTracingCategoryNameDefault>> {
 public:
  TouchstartExpectedSoonValue(bool initial_value, const void* owner)
      : TraceableValue("RendererScheduler.TouchstartExpectedSoon",
                       initial_value, owner, YesNoStateToString) {}
};

class LoadingTasksSeemsExpensiveValue : public internal::TraceableValue<bool,
                                        StateTracer<kTracingCategoryNameInfo>> {
 public:
  LoadingTasksSeemsExpensiveValue(bool initial_value, const void* owner)
      : TraceableValue("RendererScheduler.LoadingTasksSeemsExpensive",
                       initial_value, owner, YesNoStateToString) {}
};

class RendererBackgroundedValue : public internal::TraceableValue<bool,
                                  StateTracer<kTracingCategoryNameDefault>> {
 public:
  RendererBackgroundedValue(bool initial_value, const void* owner)
      : TraceableValue("RendererScheduler.Backgrounded",
                       initial_value, owner, BackgroundedStateToString) {}
};

class TimerTasksSeemsExpensiveValue : public internal::TraceableValue<bool,
                                      StateTracer<kTracingCategoryNameInfo>> {
 public:
  TimerTasksSeemsExpensiveValue(bool initial_value, const void* owner)
      : TraceableValue("RendererScheduler.TimerTasksSeemsExpensive",
                       initial_value, owner, YesNoStateToString) {}
};

class UseCaseValue : public internal::TraceableValue<UseCase,
                     StateTracer<kTracingCategoryNameDefault>> {
 public:
  UseCaseValue(UseCase initial_value, const void* owner)
      : TraceableValue("RendererScheduler.UseCase",
                       initial_value, owner, UseCaseToString) {}
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_RENDERER_SCHEDULER_TRACING_H_
