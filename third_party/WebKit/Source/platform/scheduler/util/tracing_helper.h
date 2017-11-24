// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_TRACING_HELPER_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_TRACING_HELPER_H_

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

void WarmupTracingCategories();

bool AreVerboseSnapshotsEnabled();

std::string PointerToString(const void* pointer);


template <typename T>
class ObservingTracer {
 public:
  ObservingTracer() : value_ptr_(nullptr) {}
  virtual ~ObservingTracer() {}

  void SetSource(const T* value_ptr) {
    DCHECK(value_ptr_ == nullptr);
    value_ptr_ = value_ptr;
    Trace(*value_ptr_);
  }

  void OnUpdated() {
    if (value_ptr_ != nullptr)
      Trace(*value_ptr_);
  }

 protected:
  virtual void Trace(const T& value) = 0;

 private:
  const T* value_ptr_;  // Not owned.
  DISALLOW_COPY_AND_ASSIGN(ObservingTracer);
};


template <typename T, const char* category>
class CounterTracer : public ObservingTracer<T> {
 public:
  using ConverterFuncPtr = double (*)(const T&);

  CounterTracer(const char* name,
                const void* object,
                ConverterFuncPtr converter)
     : name_(name),
       object_(object),
       converter_(converter) {
    // Category must be a constant defined in tracing_helper.
    // Unfortunately, static_assert won't work here because inequality (!=)
    // of linker symbols is undefined in compile-time.
    DCHECK(category == kTracingCategoryNameDefault ||
           category == kTracingCategoryNameInfo ||
           category == kTracingCategoryNameDebug);
  }

 private:
  void Trace(const T& value) final {
    TRACE_COUNTER_ID1(category, name_, object_, converter_(value));
  }

  const char* const name_;  // Not owned.
  const void* const object_;  // Not owned.
  const ConverterFuncPtr converter_;
};


template <typename T, const char* category>
class StateTracer : public ObservingTracer<T> {
 public:
  using ConverterFuncPtr = const char* (*)(const T);

  StateTracer(const char* name,
              const void* object,
              ConverterFuncPtr converter)
      : name_(name),
        object_(object),
        converter_(converter),
        started_(false) {
    // Category must be a constant defined in tracing_helper.
    // Unfortunately, static_assert won't work here because inequality (!=)
    // of linker symbols is undefined in compile-time.
    DCHECK(category == kTracingCategoryNameDefault ||
           category == kTracingCategoryNameInfo ||
           category == kTracingCategoryNameDebug);
  }

  virtual ~StateTracer() {
    if (started_)
      TRACE_EVENT_ASYNC_END0(category, name_, object_);
  }

 private:
  void Trace(const T& value) final {
    if (started_)
      TRACE_EVENT_ASYNC_END0(category, name_, object_);

    started_ = is_enabled();
    if (started_) {
      // Trace viewer logic relies on subslice starting at the exact same time
      // as the async event.
      base::TimeTicks now = base::TimeTicks::Now();
      TRACE_EVENT_ASYNC_BEGIN_WITH_TIMESTAMP0(category, name_, object_, now);
      TRACE_EVENT_ASYNC_STEP_INTO_WITH_TIMESTAMP0(category, name_, object_,
                                                  converter_(value), now);
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

  // We have to track |started_| state to avoid race condition since SetState
  // might be called before OnTraceLogEnabled notification.
  bool started_;

  DISALLOW_COPY_AND_ASSIGN(StateTracer);
};


template <typename T>
class ObservableValue {
 public:
  ObservableValue(T initial_value, ObservingTracer<T>* observer)
      : value_(initial_value),
        observer_(observer) {
    observer_->SetSource(&value_);
  }

  ObservableValue& operator =(const T& value) {
    Assign(value);
    return *this;
  }
  ObservableValue& operator =(const ObservableValue& another) {
    Assign(another.value_);
    return *this;
  }

  T get() const {
    return value_;
  }
  operator T() const {
    return value_;
  }

 private:
  void Assign(T new_value) {
    if (value_ != new_value) {
      value_ = new_value;
      observer_->OnUpdated();
    }
  }

  T value_;
  ObservingTracer<T>* const observer_;  // Not owned.

  DISALLOW_COPY(ObservableValue);
};

// TRACE_EVENT macros define static variable to cache a pointer to the state
// of category. Hence, we need distinct version for each category in order to
// prevent unintended leak of state.
template <typename T, const char* category>
class TraceableState {
 public:
  TraceableState(T initial_state,
                 const char* name,
                 const void* object,
                 const char* (*converter)(T))
      : name_(name),
        object_(object),
        converter_(converter),
        state_(initial_state),
        started_(false) {
    // Category must be a constant defined in tracing_helper.
    // Unfortunately, static_assert won't work here because inequality (!=)
    // of linker symbols is undefined in compile-time.
    DCHECK(category == kTracingCategoryNameDefault ||
           category == kTracingCategoryNameInfo ||
           category == kTracingCategoryNameDebug);
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
  const char* (*converter_)(T);

  T state_;
  // We have to track |started_| state to avoid race condition since SetState
  // might be called before OnTraceLogEnabled notification.
  bool started_;

  DISALLOW_COPY(TraceableState);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_TRACING_HELPER_H_
