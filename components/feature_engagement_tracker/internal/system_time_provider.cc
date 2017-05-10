// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement_tracker/internal/system_time_provider.h"

#include "base/time/time.h"

namespace feature_engagement_tracker {

SystemTimeProvider::SystemTimeProvider() = default;

SystemTimeProvider::~SystemTimeProvider() = default;

uint32_t SystemTimeProvider::GetCurrentDay() const {
  base::TimeDelta delta = base::Time::Now() - base::Time::UnixEpoch();
  return base::saturated_cast<uint32_t>(delta.InDays());
}

}  // namespace feature_engagement_tracker
