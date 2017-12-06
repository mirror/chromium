// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_TRACING_HELPER_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_TRACING_HELPER_H_

#include <memory>
#include <string>
#include "base/macros.h"
#include "base/trace_event/trace_event.h"
#include "platform/PlatformExport.h"

namespace blink {
namespace scheduler {

// DISCLAIMER
// Using these constants in TRACE_EVENTs is discouraged nor should you pass any
// non-literal string as a category, unless familiar with tracing internals.
PLATFORM_EXPORT extern const char kTracingCategoryNameDefault[];
PLATFORM_EXPORT extern const char kTracingCategoryNameInfo[];
PLATFORM_EXPORT extern const char kTracingCategoryNameDebug[];

namespace internal {

PLATFORM_EXPORT void ValidateTracingCategory(const char* category);

constexpr bool is_category_enabled(const unsigned char* category_flags) {
  return *category_flags &
      (base::trace_event::TraceCategory::ENABLED_FOR_RECORDING |
       base::trace_event::TraceCategory::ENABLED_FOR_ETW_EXPORT);
}

}  // namespace internal

PLATFORM_EXPORT void WarmupTracingCategories();

PLATFORM_EXPORT bool AreVerboseSnapshotsEnabled();

// In terms of namespaces we should also care of name collisions in anonymous
// namespaces because it may break jumbo builds (multiple .cc files get merged).
namespace util {

PLATFORM_EXPORT std::string PointerToString(const void* pointer);
PLATFORM_EXPORT const char* BackgroundStateToString(bool is_backgrounded);
PLATFORM_EXPORT const char* YesNoStateToString(bool is_yes);
PLATFORM_EXPORT double TimeDeltaToMilliseconds(const base::TimeDelta& value);

}  // namespace util


// Glue structure to link tracers with value watches.
template <typename T>
struct Glue {
  // Watch submits own callback to be invoked when OnTraceLogEnabled happens.
  base::Callback<void(base::Callback<void()>)> register_on_trace_log_enabled;
  // TraceLog owned flags to check whether category enabled or not.
  const unsigned char* category_flags;
  // Function that converts the value provided and submit trace event.
  base::Callback<void(T)> value_tracer;
};

template <typename T>
class TraceableStateX {
 public:
  TraceableStateX(T initial_state, Glue<T> glue)
      : category_flags_(glue.category_flags),
        value_tracer_(glue.value_tracer),
        state_(initial_state) {
    glue.register_on_trace_log_enabled.Run(
        base::Bind(&TraceableStateX::Trace, base::Unretained(this)));
    Trace();
  }
  ~TraceableStateX() {}

  TraceableStateX& operator =(const T& value) {
    Assign(value);
    return *this;
  }
  TraceableStateX& operator =(const TraceableStateX& another) {
    Assign(another.state_);
    return *this;
  }

  operator T() const {
    return state_;
  }

 private:
  void Assign(T new_state) {
    if (state_ != new_state) {
      state_ = new_state;
      Trace();
    }
  }

  void Trace() {
    if (internal::is_category_enabled(category_flags_)) {
      value_tracer_.Run(state_);
    }
  }

  const unsigned char* const category_flags_;
  const base::Callback<void(T)> value_tracer_;
  T state_;
};


// Opaque class which contains tracers implemented with a template magic
// unnecessary to be exposed in this header.
class RendererTracers;

// Renderer-scoped tracing manager.
class RendererTracing {
 public:
  RendererTracing(const void* object);
  ~RendererTracing();

  Glue<bool> AudioPlaying();

  void OnTraceLogEnabled();

 private:
  void RegisterOnTraceLogEnabled(base::Callback<void()> callback);

  RendererTracers* tracers_;  // Owned.
  base::Callback<void(base::Callback<void()>)> registry_;
  std::vector<base::Callback<void()>> on_trace_log_enabled_;
};

// TRACE_EVENT macros define static variable to cache a pointer to the state
// of category. Hence, we need distinct version for each category in order to
// prevent unintended leak of state.

template <typename T, const char* category>
class TraceableState {
 public:
  using ConverterFuncPtr = const char* (*)(T);

  TraceableState(T initial_state,
                 const char* name,
                 const void* object,
                 ConverterFuncPtr converter)
      : name_(name),
        object_(object),
        converter_(converter),
        state_(initial_state),
        started_(false) {
    internal::ValidateTracingCategory(category);
    Trace();
  }

  ~TraceableState() {
    if (started_)
      TRACE_EVENT_ASYNC_END0(category, name_, object_);
  }

  TraceableState& operator =(const T& value) {
    Assign(value);
    return *this;
  }
  TraceableState& operator =(const TraceableState& another) {
    Assign(another.state_);
    return *this;
  }

  operator T() const {
    return state_;
  }

  void OnTraceLogEnabled() {
    Trace();
  }

 private:
  void Assign(T new_state) {
    if (state_ != new_state) {
      state_ = new_state;
      Trace();
    }
  }

  void Trace() {
    if (started_)
      TRACE_EVENT_ASYNC_END0(category, name_, object_);

    started_ = is_enabled();
    if (started_) {
      // Trace viewer logic relies on subslice starting at the exact same time
      // as the async event.
      base::TimeTicks now = base::TimeTicks::Now();
      TRACE_EVENT_ASYNC_BEGIN_WITH_TIMESTAMP0(category, name_, object_, now);
      TRACE_EVENT_ASYNC_STEP_INTO_WITH_TIMESTAMP0(category, name_, object_,
                                                  converter_(state_), now);
    }
  }

  bool is_enabled() const {
    bool result = false;
    TRACE_EVENT_CATEGORY_GROUP_ENABLED(category, &result);  // Cached.
    return result;
  }

  const char* const name_;  // Not owned.
  const void* const object_;  // Not owned.
  const ConverterFuncPtr converter_;

  T state_;
  // We have to track |started_| state to avoid race condition since SetState
  // might be called before OnTraceLogEnabled notification.
  bool started_;

  DISALLOW_COPY(TraceableState);
};

template <typename T, const char* category>
class TraceableCounter {
 public:
  using ConverterFuncPtr = double (*)(const T&);

  TraceableCounter(T initial_value,
                   const char* name,
                   const void* object,
                   ConverterFuncPtr converter)
      : name_(name),
        object_(object),
        converter_(converter),
        value_(initial_value) {
    internal::ValidateTracingCategory(category);
    Trace();
  }

  TraceableCounter& operator =(const T& value) {
    value_ = value;
    Trace();
    return *this;
  }
  TraceableCounter& operator =(const TraceableCounter& another) {
    value_ = another.value_;
    Trace();
    return *this;
  }

  TraceableCounter& operator +=(const T& value) {
    value_ += value;
    Trace();
    return *this;
  }
  TraceableCounter& operator -=(const T& value) {
    value_ -= value;
    Trace();
    return *this;
  }

  const T& value() const {
    return value_;
  }
  const T* operator ->() const {
    return &value_;
  }
  operator T() const {
    return value_;
  }

  void Trace() const {
    TRACE_COUNTER_ID1(category, name_, object_, converter_(value_));
  }

 private:
  const char* const name_;  // Not owned.
  const void* const object_;  // Not owned.
  const ConverterFuncPtr converter_;

  T value_;
  DISALLOW_COPY(TraceableCounter);
};

// Add operators when it's needed.

template <typename T, const char* category>
constexpr T operator -(const TraceableCounter<T, category>& counter) {
  return -counter.value();
}

template <typename T, const char* category>
constexpr T operator /(const TraceableCounter<T, category>& lhs, const T& rhs) {
  return lhs.value() / rhs;
}

template <typename T, const char* category>
constexpr bool operator >(
    const TraceableCounter<T, category>& lhs, const T& rhs) {
  return lhs.value() > rhs;
}

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_TRACING_HELPER_H_
