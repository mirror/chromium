// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_COALESCING_TIMER_H_
#define MEDIA_GPU_ANDROID_COALESCING_TIMER_H_

#include <vector>

#include "base/callback.h"
#include "base/threading/thread_checker.h"
#include "base/timer/timer.h"

namespace media {

class CoalescingTimer {
 public:
  CoalescingTimer();
  ~CoalescingTimer();

  void Start(const base::Closure& callback);
  void Stop();
  const base::Closure& callback() { return callback_; }

 private:
  class Manager {
   public:
    static Manager* Instance();
    Manager();
    ~Manager() = delete;

    void AddTimer(CoalescingTimer* timer);
    void RemoveTimer(CoalescingTimer* timer);
    void RunCallbacks();

   private:
    std::vector<CoalescingTimer*> running_timers_;
    base::RepeatingTimer timer_;
    base::ThreadChecker thread_checker_;
  };

  base::Closure callback_;

  DISALLOW_COPY_AND_ASSIGN(CoalescingTimer);
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_COALESCING_TIMER_H_
