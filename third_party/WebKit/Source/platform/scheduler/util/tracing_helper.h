// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_TRACING_HELPER_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_TRACING_HELPER_H_

#include <memory>
#include "base/macros.h"

// TODO(kraynov): Move platform/scheduler/base/trace_helper.h here.

namespace blink {
namespace scheduler {
namespace scheduler_tracing {

extern const char kCategoryNameDefault[];
extern const char kCategoryNameInfo[];
extern const char kCategoryNameDebug[];

extern const char kTracerNameUseCase[];
extern const char kTracerNameBackgrounding[];
extern const char kTracerNameAudioPlaying[];

void WarmupCategories();
bool AreVerboseSnapshotsEnabled();

class StateTracer {
 public:
  virtual void SetState(const char* state) = 0;
  virtual ~StateTracer() {}

 protected:
  StateTracer() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(StateTracer);
};

class StateTracers {
 public:
  static std::unique_ptr<StateTracer> CreateUseCase(const void* object);
  static std::unique_ptr<StateTracer> CreateBackgrounding(const void* object);
  static std::unique_ptr<StateTracer> CreateAudioPlaying(const void* object);
};

}  // namespace scheduler_tracing
}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_TRACING_HELPER_H_
