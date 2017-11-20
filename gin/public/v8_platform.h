// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GIN_PUBLIC_V8_PLATFORM_H_
#define GIN_PUBLIC_V8_PLATFORM_H_

#include "base/compiler_specific.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/optional.h"
#include "gin/gin_export.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"
#include "v8/include/v8-platform.h"

namespace gin {

// A v8::Platform implementation to use with gin.
class GIN_EXPORT V8Platform : public v8::Platform {
 public:
  static V8Platform* Get();

  // Forces the Platform to report specified time and timezone instead of the
  // real one. If value is missing override for this value is cleared. Time and
  // timezone can be overriden independently.
  void SetCurrentTimeOverride(const base::Optional<double> time_millis,
                              const base::Optional<std::string> time_zone);

  // v8::Platform implementation.
  void OnCriticalMemoryPressure() override;
  std::shared_ptr<v8::TaskRunner> GetForegroundTaskRunner(
      v8::Isolate*) override;
  std::shared_ptr<v8::TaskRunner> GetBackgroundTaskRunner(
      v8::Isolate*) override;
  size_t NumberOfAvailableBackgroundThreads() override;
  void CallOnBackgroundThread(
      v8::Task* task,
      v8::Platform::ExpectedRuntime expected_runtime) override;
  void CallOnForegroundThread(v8::Isolate* isolate, v8::Task* task) override;
  void CallDelayedOnForegroundThread(v8::Isolate* isolate,
                                     v8::Task* task,
                                     double delay_in_seconds) override;
  void CallIdleOnForegroundThread(v8::Isolate* isolate,
                                  v8::IdleTask* task) override;
  bool IdleTasksEnabled(v8::Isolate* isolate) override;
  double MonotonicallyIncreasingTime() override;
  double CurrentClockTimeMillis() override;
  StackTracePrinter GetStackTracePrinter() override;
  v8::TracingController* GetTracingController() override;

 private:
  friend struct base::LazyInstanceTraitsBase<V8Platform>;

  V8Platform();
  ~V8Platform() override;

  class TracingControllerImpl;
  std::unique_ptr<TracingControllerImpl> tracing_controller_;
  base::Optional<double> clock_time_in_millis_override_;
  std::unique_ptr<icu::TimeZone> saved_time_zone_;

  DISALLOW_COPY_AND_ASSIGN(V8Platform);
};

}  // namespace gin

#endif  // GIN_PUBLIC_V8_PLATFORM_H_
