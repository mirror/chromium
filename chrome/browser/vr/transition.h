// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TRANSITION_H_
#define CHROME_BROWSER_VR_TRANSITION_H_

#include <set>

#include "base/time/time.h"
#include "cc/animation/timing_function.h"

namespace vr {

struct Transition {
  Transition();
  ~Transition();

  // TODO(vollick): would be nice to have a delay.
  base::TimeDelta duration;
  std::set<int> target_properties;
  cc::CubicBezierTimingFunction::EaseType ease_type;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_TRANSITION_H_
