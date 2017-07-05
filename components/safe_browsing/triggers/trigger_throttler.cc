// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/triggers/trigger_throttler.h"

#include "base/stl_util.h"
#include "base/time/time.h"

namespace safe_browsing {

namespace {
const int kUnlimitedTriggerQuota = -1;

int GetDailyQuotaForTrigger(const SafeBrowsingTriggerType type) {
  switch (type) {
    case SafeBrowsingTriggerType::SECURITY_INTERSTITIAL:
      return kUnlimitedTriggerQuota;
  }
}

}  // namespace

TriggerThrottler::TriggerThrottler() {}

TriggerThrottler::~TriggerThrottler() {}

bool TriggerThrottler::CheckQuota(const SafeBrowsingTriggerType type) const {
  LOG(ERROR)
      << "Lpz: checkquota, t="
      << static_cast<std::underlying_type<SafeBrowsingTriggerType>::type>(type);
  // Lookup how many times this trigger is allowed to fire each day.
  const int trigger_quota = GetDailyQuotaForTrigger(type);

  // Some basic corner cases for triggers that always fire, or disabled
  // triggers that never fire.
  if (trigger_quota == kUnlimitedTriggerQuota)
    return true;
  if (trigger_quota == 0)
    return false;

  // Other triggers are capped, see how many times this trigger has already
  // fired.
  if (!base::ContainsKey(trigger_events_, type))
    return true;

  const std::vector<time_t>& timestamps = trigger_events_.at(type);
  // More quota is available, so the trigger can fire again.
  if (trigger_quota > static_cast<int>(timestamps.size()))
    return true;

  // Otherwise, we have more events than quota, check which day they occured on.
  // Newest events are at the end of vector so we can simply look at the
  // Nth-from-last entry (where N is the quota) to see if it happened within
  // the current day or earlier.
  base::Time min_timestamp = base::Time::Now() - base::TimeDelta::FromDays(1);
  const size_t pos = timestamps.size() - trigger_quota + 1;
  return timestamps[pos] < min_timestamp.ToTimeT();
}

void TriggerThrottler::TriggerFired(const SafeBrowsingTriggerType type) {
  LOG(ERROR)
      << "Lpz: triggerfired, t="
      << static_cast<std::underlying_type<SafeBrowsingTriggerType>::type>(type);
  // Lookup how many times this trigger is allowed to fire each day.
  const int trigger_quota = GetDailyQuotaForTrigger(type);

  // For triggers that always fire, don't bother tracking quota.
  if (trigger_quota == kUnlimitedTriggerQuota)
    return;

  // Otherwise, record that the trigger fired.
  std::vector<time_t>* timestamps = &trigger_events_[type];
  timestamps->push_back(base::Time::Now().ToTimeT());

  // Clean up the trigger events map.
  CleanupOldEvents();
}

void TriggerThrottler::CleanupOldEvents() {
  for (const auto& map_iter : trigger_events_) {
    const SafeBrowsingTriggerType trigger_type = map_iter.first;

    const int trigger_quota = GetDailyQuotaForTrigger(trigger_type);
    base::Time min_timestamp = base::Time::Now() - base::TimeDelta::FromDays(1);

    // Go over the event times for this trigger and keep up to |trigger_quota|
    // timestamps which are newer than |min_timestamp|.
    // We put timestamps in a temp vector that will get swapped into the map in
    // place of the existing one.
    const std::vector<time_t>& trigger_times = map_iter.second;
    std::vector<time_t> tmp_trigger_times;
    for (const time_t timestamp : trigger_times) {
      // If we have enough events, stop.
      if (static_cast<int>(tmp_trigger_times.size()) >= trigger_quota)
        break;

      // See if this event time is new enough to be considered.
      if (timestamp > min_timestamp.ToTimeT())
        tmp_trigger_times.push_back(timestamp);
    }

    trigger_events_[trigger_type] = tmp_trigger_times;
  }
}

}  // namespace safe_browsing
