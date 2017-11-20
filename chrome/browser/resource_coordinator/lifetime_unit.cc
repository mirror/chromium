// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/lifetime_unit.h"

namespace resource_coordinator {

LifetimeUnit::SortKey::SortKey() = default;

LifetimeUnit::SortKey::SortKey(base::TimeTicks last_focused_time)
    : last_focused_time(last_focused_time) {}

bool LifetimeUnit::SortKey::operator<(const SortKey& other) const {
  return last_focused_time < other.last_focused_time;
}

bool LifetimeUnit::SortKey::operator>(const SortKey& other) const {
  return last_focused_time > other.last_focused_time;
}

}  // namespace resource_coordinator
