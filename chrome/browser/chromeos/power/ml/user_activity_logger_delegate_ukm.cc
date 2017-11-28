// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>
#include <vector>

#include "chrome/browser/chromeos/power/ml/user_activity_event.pb.h"
#include "chrome/browser/chromeos/power/ml/user_activity_logger_delegate_ukm.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_entry_builder.h"

namespace chromeos {
namespace power {
namespace ml {

namespace {

const int kBatteryPercentBuckets[] = {0,  5,  10, 15, 20, 25, 30,
                                      35, 40, 45, 50, 55, 60, 65,
                                      70, 75, 80, 85, 90, 95, 100};

}  // namespace

// static
int UserActivityLoggerDelegateUkm::BucketEveryFivePercents(int original_value) {
  DCHECK_GE(original_value, 0);
  DCHECK_LE(original_value, 100);
  const int* upper =
      std::upper_bound(std::begin(kBatteryPercentBuckets),
                       std::end(kBatteryPercentBuckets), original_value);
  return *(upper - 1);
}

UserActivityLoggerDelegateUkm::UserActivityLoggerDelegateUkm()
    : ukm_recorder_(ukm::UkmRecorder::Get()) {
  DCHECK(ukm_recorder_);
}

UserActivityLoggerDelegateUkm::~UserActivityLoggerDelegateUkm() = default;

void UserActivityLoggerDelegateUkm::UpdateOpenTabsURLs() {
  source_ids_.clear();
  for (Browser* browser : *BrowserList::GetInstance()) {
    const TabStripModel* const tab_strip_model = browser->tab_strip_model();
    DCHECK(tab_strip_model);
    for (int i = 0; i < tab_strip_model->count(); ++i) {
      const content::WebContents* const contents =
          tab_strip_model->GetWebContentsAt(i);
      DCHECK(contents);

      const GURL url = contents->GetLastCommittedURL();
      // TODO(jiameng): once UKM is more complete, we will use existing source
      // IDs for these navigations. For the moment, we're creating new source
      // IDs and record dummy Entry.
      ukm::SourceId source_id = ukm_recorder_->GetNewSourceID();
      ukm_recorder_->UpdateSourceURL(source_id, url);
      ukm::builders::UserActivity user_activity(source_id);
      user_activity.Record(ukm_recorder_);
      source_ids_.push_back(source_id);
    }
  }
}

void UserActivityLoggerDelegateUkm::LogActivity(
    const UserActivityEvent& event) {
  ukm::SourceId source_id = ukm_recorder_->GetNewSourceID();
  ukm::builders::UserActivity user_activity(source_id);
  user_activity.SetEventType(event.event().type())
      .SetEventReason(event.event().reason())
      .SetLastActivityTime(event.features().last_activity_time_sec())
      .SetLastActivityDay(event.features().last_activity_day())
      .SetRecentTimeActive(event.features().recent_time_active_sec())
      .SetDeviceType(event.features().device_type())
      .SetDeviceMode(event.features().device_mode())
      .SetOnBattery(event.features().on_battery());

  if (event.features().has_last_user_activity_time_sec()) {
    user_activity.SetLastUserActivityTime(
        event.features().last_user_activity_time_sec());
  }
  if (event.features().has_time_since_last_mouse_sec()) {
    user_activity.SetTimeSinceLastMouse(
        event.features().time_since_last_mouse_sec());
  }
  if (event.features().has_time_since_last_key_sec()) {
    user_activity.SetTimeSinceLastKey(
        event.features().time_since_last_key_sec());
  }
  user_activity.SetBatteryPercent(
      BucketEveryFivePercents(std::floor(event.features().battery_percent())));

  for (const ukm::SourceId& id : source_ids_) {
    user_activity.SetURLSourceId(id);
  }

  user_activity.Record(ukm_recorder_);
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
