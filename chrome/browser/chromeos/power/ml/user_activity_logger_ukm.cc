// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>
#include <vector>

#include "chrome/browser/chromeos/power/ml/user_activity_event.pb.h"
#include "chrome/browser/chromeos/power/ml/user_activity_logger_ukm.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/web_contents.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_entry_builder.h"

namespace chromeos {
namespace power {
namespace ml {

UserActivityLoggerUKM::UserActivityLoggerUKM() {
  ukm_recorder_ = ukm::UkmRecorder::Get();
  DCHECK(ukm_recorder_);
}

UserActivityLoggerUKM::~UserActivityLoggerUKM() {}

void UserActivityLoggerUKM::GetOpenTabsURLs() {
  DCHECK(source_ids_.empty());
  for (auto* browser : *BrowserList::GetInstance()) {
    const TabStripModel* const tab_strip_model = browser->tab_strip_model();
    DCHECK(tab_strip_model);
    for (int i = 0; i < tab_strip_model->count(); ++i) {
      const content::WebContents* const contents =
          tab_strip_model->GetWebContentsAt(i);
      if (contents) {
        const GURL url = contents->GetLastCommittedURL();
        ukm::SourceId source_id = ukm_recorder_->GetNewSourceID();
        ukm_recorder_->UpdateSourceURL(source_id, url);
        source_ids_.push_back(source_id);
      }
    }
  }
}

void UserActivityLoggerUKM::LogActivity(const UserActivityEvent& event) {
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
  const float multiple = event.features().battery_percent() / 5;
  user_activity.SetBatteryPercent(5 * std::lround(multiple));

  for (const ukm::SourceId& id : source_ids_) {
    user_activity.SetURLSourceId(id);
  }

  user_activity.Record(ukm_recorder_);
  source_ids_.clear();
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
