// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_METRICS_HELPER_H_
#define CHROME_BROWSER_VR_METRICS_HELPER_H_

#include <string>

#include "base/optional.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"

namespace vr {

// Specifies one of Chrome's VR modes.
enum class Mode : int {
  kVr = 0,
  kVrBrowsing,
  kWebVr,
  kCount,
};

// Helper to collect VR UMA metrics.
//
// For thread-safety, all functions must be called in sequence.
class MetricsHelper {
 public:
  MetricsHelper();
  ~MetricsHelper();

  void OnComponentReady();
  void OnEnter(Mode mode);

 private:
  template <Mode ModeName>
  void OnEnter();
  base::Optional<base::Time>& GetEnterTime(Mode mode);
  template <Mode ModeName>
  void LogWaitLatencyIfWaited(const base::Time& now);

  base::Optional<base::Time> enter_vr_time_;
  base::Optional<base::Time> enter_vr_browsing_time_;
  base::Optional<base::Time> enter_web_vr_time_;
  bool component_ready_ = false;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_METRICS_HELPER_H_
