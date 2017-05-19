// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_AVAILABILITY_STORE_H_
#define COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_AVAILABILITY_STORE_H_

#include <stdint.h>

#include <map>
#include <memory>

#include "base/callback.h"
#include "base/macros.h"

namespace base {
struct Feature;
}  // namespace base

namespace feature_engagement_tracker {

// An AvailabilityStore provides a way to load the availability date for all
// available features.
class AvailabilityStore {
 public:
  // Invoked when the availability data has finished loading, and whether the
  // load was a success. In the case of a failure, the map argument will be
  // empty. The value for each entry in the map is the day number since epoch
  // (1970-01-01) in the local timezone for when the particular feature was made
  // available.
  using OnLoadedCallback = base::Callback<void(
      bool success,
      std::unique_ptr<std::map<const base::Feature*, uint32_t>>)>;

  virtual ~AvailabilityStore() = default;

  // Loads the availability data and asynchronously invokes |callback| with the
  // result.
  virtual void Load(const OnLoadedCallback& callback) = 0;

 protected:
  AvailabilityStore() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(AvailabilityStore);
};

}  // namespace feature_engagement_tracker

#endif  // COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_AVAILABILITY_STORE_H_
