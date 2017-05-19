// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_AVAILABILITY_TRACKER_IMPL_H_
#define COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_AVAILABILITY_TRACKER_IMPL_H_

#include <stdint.h>

#include <map>
#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/feature_engagement_tracker/internal/availability_tracker.h"
#include "components/feature_engagement_tracker/public/feature_list.h"

namespace base {
struct Feature;
}  // namespace base

namespace feature_engagement_tracker {
class AvailabilityStore;

// An AvailabilityTracker tracks when each feature was made available to an
// end user.
class AvailabilityTrackerImpl : public AvailabilityTracker {
 public:
  // Invoked when the availability data has finished loading, and whether the
  // load was a success. In the case of a failure, it is invalid to ever call
  // GetAvailability(...).
  using OnLoadedCallback = base::Callback<void(bool success)>;

  // The |store| is used to load all the availability data.
  // The |feature_filter| is used to filter the data from the |store| and ensure
  // that only features listed in this filter are tracked in this class. For
  // features that are in the |feature_filter|, but not in the |store|, they are
  // added as known, but with an availability time of 0u.
  AvailabilityTrackerImpl(std::unique_ptr<AvailabilityStore> store,
                          std::unique_ptr<FeatureVector> feature_filter);
  ~AvailabilityTrackerImpl() override;

  // Loads all the availabilities, and stores them for later retrieval. It is
  // only valid to call this method once. The callback is guaranteed to be
  // invoked asynchronously,
  void Load(const OnLoadedCallback& callback);

  // Returns true iff the given |feature| is part of the known features.
  bool HasAvailabilityForTest(const base::Feature& feature) const;

  // Returns true iff the feature filter still exists.
  bool HasFilterForTest() const;

  // AvailabilityTracker implementation.
  uint32_t GetAvailability(const base::Feature& feature) const override;

 private:
  // This is invoked when the store has completed loading.
  void OnStoreLoadComplete(
      const OnLoadedCallback& callback,
      bool success,
      std::unique_ptr<std::map<const base::Feature*, uint32_t>>);

  // Cleans up all unnecessary members after the load from the store has
  // finished.
  void CleanupAfterLoad();

  // The underlying store. This will be destructed after the initial load has
  // been completed.
  std::unique_ptr<AvailabilityStore> store_;

  // Stores the day number since epoch (1970-01-01) in the local timezone for
  // when the particular |feature| was made available.
  std::map<const base::Feature*, uint32_t> feature_availabilities_;

  // In case the database contains more features than what is required, this
  // filter is used to ensure that |feature_availabilities_| only contains the
  // features that are listed in this filter. This will be destructed after
  // the initial load has been completed.
  std::unique_ptr<FeatureVector> feature_filter_;

  base::WeakPtrFactory<AvailabilityTrackerImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AvailabilityTrackerImpl);
};

}  // namespace feature_engagement_tracker

#endif  // COMPONENTS_FEATURE_ENGAGEMENT_TRACKER_INTERNAL_AVAILABILITY_TRACKER_IMPL_H_
