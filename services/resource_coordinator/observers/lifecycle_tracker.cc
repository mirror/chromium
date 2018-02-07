// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/observers/lifecycle_tracker.h"

#include "services/resource_coordinator/coordination_unit/coordination_unit_base.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_id.h"
#include "services/resource_coordinator/public/cpp/coordination_unit_types.h"

namespace resource_coordinator {

LifecycleTracker::LifecycleTracker() = default;

LifecycleTracker::~LifecycleTracker() = default;

bool LifecycleTracker::ShouldObserve(
    const CoordinationUnitBase* coordination_unit) {
  return coordination_unit->id().type == CoordinationUnitType::kPage;
}

void LifecycleTracker::OnPagePropertyChanged(
    const PageCoordinationUnitImpl* page_cu,
    const mojom::PropertyType property_type,
    int64_t value) {
  // LOG(ERROR) << "LifecycleTracker::OnPagePropertyChanged: " << property_type
  //            << ", " << value;
}

void LifecycleTracker::UpdateDiscardUI() {}

}  // namespace resource_coordinator
