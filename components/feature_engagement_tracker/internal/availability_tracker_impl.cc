// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement_tracker/internal/availability_tracker_impl.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "components/feature_engagement_tracker/internal/availability_store.h"
#include "components/feature_engagement_tracker/public/feature_list.h"

namespace feature_engagement_tracker {

AvailabilityTrackerImpl::AvailabilityTrackerImpl(
    std::unique_ptr<AvailabilityStore> store,
    std::unique_ptr<FeatureVector> feature_filter)
    : store_(std::move(store)),
      feature_filter_(std::move(feature_filter)),
      weak_ptr_factory_(this) {}

AvailabilityTrackerImpl::~AvailabilityTrackerImpl() = default;

void AvailabilityTrackerImpl::Load(const OnLoadedCallback& callback) {
  store_->Load(base::Bind(&AvailabilityTrackerImpl::OnStoreLoadComplete,
                          weak_ptr_factory_.GetWeakPtr(), callback));
}

bool AvailabilityTrackerImpl::HasAvailabilityForTest(
    const base::Feature& feature) const {
  return feature_availabilities_.find(&feature) !=
         feature_availabilities_.end();
}

void AvailabilityTrackerImpl::OnStoreLoadComplete(
    const OnLoadedCallback& callback,
    bool success,
    std::unique_ptr<std::map<const base::Feature*, uint32_t>>
        feature_availabilities) {
  if (!success) {
    callback.Run(false);
    return;
  }

  // Only extract availability data for features in the |feature_filter_|.
  for (auto it = (*feature_availabilities).begin();
       it != (*feature_availabilities).end(); ++it) {
    if (std::find((*feature_filter_).begin(), (*feature_filter_).end(),
                  it->first) != (*feature_filter_).end()) {
      feature_availabilities_[it->first] = it->second;
    }
  }

  // For features listed in |feature_filter_|, ensure they will be in the
  // availability data by inserting features that are not already tracked
  // with a value of 0u.
  for (auto it = (*feature_filter_).begin(); it != (*feature_filter_).end();
       ++it) {
    if (feature_availabilities_.find(*it) == feature_availabilities_.end()) {
      feature_availabilities_[*it] = 0u;
    }
  }

  callback.Run(true);
}

uint32_t AvailabilityTrackerImpl::GetAvailability(
    const base::Feature& feature) const {
  auto search = feature_availabilities_.find(&feature);
  DCHECK(search != feature_availabilities_.end());
  return search->second;
}

}  // namespace feature_engagement_tracker
