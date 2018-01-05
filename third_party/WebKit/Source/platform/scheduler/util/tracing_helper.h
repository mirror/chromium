// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_TRACING_HELPER_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_TRACING_HELPER_H_

#include <string>
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/time/time.h"
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

class TraceableStateForTest;

}  // namespace internal

PLATFORM_EXPORT void WarmupTracingCategories();

PLATFORM_EXPORT bool AreVerboseSnapshotsEnabled();

PLATFORM_EXPORT std::string PointerToString(const void* pointer);

PLATFORM_EXPORT double TimeDeltaToMilliseconds(const base::TimeDelta& value);

class TraceableVariable {
 public:
  virtual ~TraceableVariable() {}
  virtual void OnTraceLogEnabled() = 0;
};

// Owner classes of |TraceableState| or |TraceableCounter| objects should
// implement this interface and take care of invoking OnTraceLogEnabled
// when it happens.
// Unfortunately, using |base::trace_event::TraceLog::EnabledStateObserver|
// wouldn't be helpful in this case because removing one takes linear time
// and tracers may be created and disposed frequently.
class TraceableVariableController {
 public:
  virtual void RegisterTraceableVariable(TraceableVariable*) = 0;
};

// TRACE_EVENT macros define static variable to cache a pointer to the state
// of category. Hence, we need distinct version for each category in order to
// prevent unintended leak of state.

template <typename T, const char* category>
class TraceableState : public TraceableVariable {
 public:
  using ConverterFuncPtr = const char* (*)(T);

  TraceableState(T initial_state,
                 const char* name,
                 TraceableVariableController* controller,
                 ConverterFuncPtr converter)
      : name_(name),
        object_(controller),
        converter_(converter),
        state_(initial_state),
        slice_is_open_(false) {
    internal::ValidateTracingCategory(category);
    controller->RegisterTraceableVariable(this);
    Trace();
  }

  ~TraceableState() override {
    if (slice_is_open_)
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
  const T& get() const {
    return state_;
  }

  void OnTraceLogEnabled() final {
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
    if (UNLIKELY(mock_trace_for_test_)) {
      mock_trace_for_test_(converter_(state_));
      return;
    }

    if (slice_is_open_) {
      TRACE_EVENT_ASYNC_END0(category, name_, object_);
      slice_is_open_ = false;
    }
    if (!is_enabled())
      return;
    // Converter returns nullptr to indicate the absence of state.
    const char* state_str = converter_(state_);
    if (!state_str)
      return;

    // Trace viewer logic relies on subslice starting at the exact same time
    // as the async event.
    base::TimeTicks now = base::TimeTicks::Now();
    TRACE_EVENT_ASYNC_BEGIN_WITH_TIMESTAMP0(category, name_, object_, now);
    TRACE_EVENT_ASYNC_STEP_INTO_WITH_TIMESTAMP0(category, name_, object_,
                                                state_str, now);
    slice_is_open_ = true;
  }

  bool is_enabled() const {
    bool result = false;
    TRACE_EVENT_CATEGORY_GROUP_ENABLED(category, &result);  // Cached.
    return result;
  }

  const char* const name_;  // Not owned.
  const void* const object_;  // Not owned.
  const ConverterFuncPtr converter_;

  void (*mock_trace_for_test_)(const char*) = nullptr;

  T state_;
  // We have to track whether slice is open to avoid confusion since assignment,
  // "absent" state and OnTraceLogEnabled can happen anytime.
  bool slice_is_open_;

  friend class internal::TraceableStateForTest;
  DISALLOW_COPY(TraceableState);
};

template <typename T, const char* category>
class TraceableCounter : public TraceableVariable {
 public:
  using ConverterFuncPtr = double (*)(const T&);

  TraceableCounter(T initial_value,
                   const char* name,
                   TraceableVariableController* controller,
                   ConverterFuncPtr converter)
      : name_(name),
        object_(controller),
        converter_(converter),
        value_(initial_value) {
    internal::ValidateTracingCategory(category);
    controller->RegisterTraceableVariable(this);
    Trace();
  }

  TraceableCounter(T initial_value,
                   const char* name,
                   TraceableVariableController* controller)
      : name_(name),
        object_(controller),
        converter_([](const T& value) { return static_cast<double>(value); }),
        value_(initial_value) {
    internal::ValidateTracingCategory(category);
    controller->RegisterTraceableVariable(this);
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

  void OnTraceLogEnabled() final {
    Trace();
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

template <typename T, const char* category>
constexpr bool operator <(
    const TraceableCounter<T, category>& lhs, const T& rhs) {
  return lhs.value() < rhs;
}

template <typename T, const char* category>
constexpr bool operator !=(
    const TraceableCounter<T, category>& lhs, const T& rhs) {
  return lhs.value() != rhs;
}

template <typename T, const char* category>
constexpr T operator ++(TraceableCounter<T, category>& counter) {
  counter = counter.value() + 1;
  return counter.value();
}

template <typename T, const char* category>
constexpr T operator --(TraceableCounter<T, category>& counter) {
  counter = counter.value() - 1;
  return counter.value();
}

template <typename T, const char* category>
constexpr T operator ++(TraceableCounter<T, category>& counter, int) {
  T value = counter.value();
  counter = value + 1;
  return value;
}

template <typename T, const char* category>
constexpr T operator --(TraceableCounter<T, category>& counter, int) {
  T value = counter.value();
  counter = value - 1;
  return value;
}

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_TRACING_HELPER_H_
