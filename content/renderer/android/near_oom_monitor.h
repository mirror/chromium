// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_ANDROID_NEAR_OOM_MONITOR_H_
#define CONTENT_RENDERER_ANDROID_NEAR_OOM_MONITOR_H_

#include "base/process/process_metrics.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/common/content_export.h"
#include "third_party/WebKit/public/web/WebPageSuspender.h"

namespace content {

class CONTENT_EXPORT NearOomMonitor {
 public:
  static std::unique_ptr<NearOomMonitor> Create();

  NearOomMonitor();
  ~NearOomMonitor();

  void Start();

  void ResumeIfNeeded();

 private:
  void Check();

  std::unique_ptr<base::ProcessMetrics> metrics_;
  int64_t threshold_;
  base::TimeDelta monitoring_interval_;
  base::RepeatingTimer timer_;

  std::unique_ptr<blink::WebPageSuspender> suspender_;
  bool resumed_;

  DISALLOW_COPY_AND_ASSIGN(NearOomMonitor);
};

}  // namespace content

#endif  // CONTENT_RENDERER_ANDROID_NEAR_OOM_MONITOR_H_
