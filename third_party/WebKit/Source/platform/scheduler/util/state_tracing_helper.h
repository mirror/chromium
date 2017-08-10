// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_STATE_TRACING_HELPER_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_STATE_TRACING_HELPER_H_

#include "base/macros.h"

namespace blink {
namespace scheduler {

// Helper class to visualize a state of an object using async trace events.
// It relies on the caller to listen to OnTraceLogEnabled and call Start.
class StateTracingHelper {
 public:
  StateTracingHelper(const char* tracing_category,
                     const char* name,
                     void* object,
                     const char* state);
  ~StateTracingHelper();

  void Start(const char* state);
  void SetState(const char* state);

 private:
  const char* const tracing_category_;
  const char* const name_;
  const void* const object_;

  DISALLOW_COPY_AND_ASSIGN(StateTracingHelper);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_STATE_TRACING_HELPER_H_
