// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_OBSERVER_LIFECYCLE_TRACKER_H_
#define SERVICES_RESOURCE_COORDINATOR_OBSERVER_LIFECYCLE_TRACKER_H_

#include "services/resource_coordinator/observers/coordination_unit_graph_observer.h"

namespace resource_coordinator {

class LifecycleTracker : public CoordinationUnitGraphObserver {
 public:
  LifecycleTracker();
  ~LifecycleTracker() override;
  // CoordinationUnitGraphObserver implementation.
  bool ShouldObserve(const CoordinationUnitBase* coordination_unit) override;
  void OnPagePropertyChanged(const PageCoordinationUnitImpl* page_cu,
                             const mojom::PropertyType property_type,
                             int64_t value) override;

 private:
  void UpdateDiscardUI();

  DISALLOW_COPY_AND_ASSIGN(LifecycleTracker);
};

}  // namespace resource_coordinator

#endif  // SERVICES_RESOURCE_COORDINATOR_OBSERVER_LIFECYCLE_TRACKER_H_
