// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEMORY_NEAR_OOM_DETECTOR_ANDROID_H_
#define CONTENT_BROWSER_MEMORY_NEAR_OOM_DETECTOR_ANDROID_H_

#include "base/cancelable_callback.h"
#include "base/time/time.h"
#include "content/common/content_export.h"
#include "content/common/renderer.mojom.h"

namespace content {

class MemoryMonitorAndroid;

using OutOfMemoryCondition = mojom::OutOfMemoryCondition;

class CONTENT_EXPORT NearOOMDetector {
 public:
  static NearOOMDetector* GetInstance();

  bool Start();

 private:
  NearOOMDetector();
  ~NearOOMDetector();

  void ScheduleCheck(base::TimeDelta delay);
  void Check();
  OutOfMemoryCondition CalculateCondition(int64_t cached);

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  std::unique_ptr<MemoryMonitorAndroid> memory_monitor_;
  base::CancelableClosure check_closure_;
  base::TimeDelta monitoring_interval_;
  base::TimeDelta cooldown_interval_;
  int64_t near_oom_threshold_;
  OutOfMemoryCondition last_condition_ = OutOfMemoryCondition::NORMAL;

  DISALLOW_COPY_AND_ASSIGN(NearOOMDetector);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEMORY_NEAR_OOM_DETECTOR_ANDROID_H_
