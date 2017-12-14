// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_THREAD_CPU_THROTTLER_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_THREAD_CPU_THROTTLER_H_

#include <memory>

#include "base/macros.h"
#include "platform/PlatformExport.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
};

namespace blink {
namespace scheduler {

// Singleton that manages creation of the throttler thread.
class PLATFORM_EXPORT ThreadCPUThrottler final {
 public:
  void SetThrottlingRate(double rate);
  static ThreadCPUThrottler* GetInstance();

 private:
  class ThrottlingThread;

  ThreadCPUThrottler();
  ~ThreadCPUThrottler();
  friend struct base::DefaultSingletonTraits<ThreadCPUThrottler>;

  std::unique_ptr<ThrottlingThread> throttling_thread_;

  DISALLOW_COPY_AND_ASSIGN(ThreadCPUThrottler);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_UTIL_THREAD_CPU_THROTTLER_H_
