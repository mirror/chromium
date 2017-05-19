// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_AVAILABILITY_TRACKER_H_
#define COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_AVAILABILITY_TRACKER_H_

#include <stdint.h>

#include "base/macros.h"

namespace base {
struct Feature;
}  // namespace base

namespace feature_engagement_tracker {

// An AvailabilityTracker tracks when each feature was made available to an
// end user.
class AvailabilityTracker {
 public:
  virtual ~AvailabilityTracker() = default;

  // Returns the day number since epoch (1970-01-01) in the local timezone for
  // when the particular |feature| was made available.
  // See TimeProvider::GetCurrentDay().
  virtual uint32_t GetAvailability(const base::Feature& feature) const = 0;

 protected:
  AvailabilityTracker() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(AvailabilityTracker);
};

}  // namespace feature_engagement_tracker

#endif  // COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_AVAILABILITY_TRACKER_H_
