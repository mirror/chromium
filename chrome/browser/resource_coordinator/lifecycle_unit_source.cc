// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/lifecycle_unit_source.h"

#include "chrome/browser/resource_coordinator/lifecycle_unit_source_observer.h"

namespace resource_coordinator {

LifecycleUnitSource::LifecycleUnitSource() = default;
LifecycleUnitSource::~LifecycleUnitSource() = default;

void LifecycleUnitSource::AddObserver(LifecycleUnitSourceObserver* observer) {
  observers_.AddObserver(observer);
}

void LifecycleUnitSource::RemoveObserver(
    LifecycleUnitSourceObserver* observer) {
  observers_.RemoveObserver(observer);
}

void LifecycleUnitSource::NotifyLifecycleUnitCreated(
    LifecycleUnit* lifecycle_unit) {
  for (LifecycleUnitSourceObserver& observer : observers_)
    observer.OnLifecycleUnitCreated(lifecycle_unit);
}

void LifecycleUnitSource::NotifyLifecycleUnitDestroyed(
    LifecycleUnit* lifecycle_unit) {
  for (LifecycleUnitSourceObserver& observer : observers_)
    observer.OnLifecycleUnitDestroyed(lifecycle_unit);
}

}  // namespace resource_coordinator
