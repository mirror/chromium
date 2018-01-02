// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/user_activity_logger_delegate_ukm.h"

#include <memory>
#include <vector>

#include "chrome/browser/chromeos/power/ml/user_activity_event.pb.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_ukm_test_helper.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/web_contents_tester.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace power {
namespace ml {

using ukm::builders::UserActivity;
using ukm::builders::UserActivityId;
using content::WebContentsTester;

const char* kUserActivityEntryName = UserActivity::kEntryName;
const char* kUserActivityIdEntryName = UserActivityId::kEntryName;

const GURL kTestUrls[] = {GURL("https://example1.com/"),
                          GURL("https://example2.com/")};

using UkmMetricMap = std::map<const char*, int64_t>;

const UkmMetricMap kUseActivityValues(
    {{UserActivity::kEventTypeName, 1},    // REACTIVATE
     {UserActivity::kEventReasonName, 1},  // USER_ACTIVITY
     {UserActivity::kLastActivityTimeName, 2},
     {UserActivity::kLastUserActivityTimeName, 1},
     {UserActivity::kLastActivityDayName, 1},  // MON
     {UserActivity::kTimeSinceLastMouseName, 100},
     {UserActivity::kRecentTimeActiveName, 10},
     {UserActivity::kDeviceTypeName, 1},  // CHROMEBOOK
     {UserActivity::kDeviceModeName, 2},  // CLAMSHELL
     {UserActivity::kBatteryPercentName, 95}});

class UserActivityLoggerDelegateUkmTest : public TabActivityTestBase {
 protected:
  UserActivityLoggerDelegateUkmTest() {
    UserActivityEvent::Event* event = user_activity_event_.mutable_event();
    event->set_type(UserActivityEvent::Event::REACTIVATE);
    event->set_reason(UserActivityEvent::Event::USER_ACTIVITY);
    UserActivityEvent::Features* features =
        user_activity_event_.mutable_features();
    features->set_last_activity_time_sec(730);
    features->set_last_user_activity_time_sec(380);
    features->set_last_activity_day(UserActivityEvent::Features::MON);
    features->set_time_since_last_mouse_sec(100);
    features->set_recent_time_active_sec(10);
    features->set_device_type(UserActivityEvent::Features::CHROMEBOOK);
    features->set_device_mode(UserActivityEvent::Features::CLAMSHELL);
    features->set_battery_percent(96.0);
  }

  UkmEntryChecker* ukm_entry_checker() { return &ukm_entry_checker_; }

  void UpdateOpenTabsURLs() {
    user_activity_logger_delegate_ukm_.UpdateOpenTabsURLs();
  }

  void LogActivity() {
    user_activity_logger_delegate_ukm_.LogActivity(user_activity_event_);
  }

 private:
  UkmEntryChecker ukm_entry_checker_;
  UserActivityEvent user_activity_event_;
  UserActivityLoggerDelegateUkm user_activity_logger_delegate_ukm_;
};

TEST_F(UserActivityLoggerDelegateUkmTest, CheckValues) {
  const std::vector<int> original_values = {0, 14, 15, 100};
  const std::vector<int> buckets = {0, 10, 15, 100};
  for (size_t i = 0; i < original_values.size(); ++i) {
    EXPECT_EQ(buckets[i],
              UserActivityLoggerDelegateUkm::BucketEveryFivePercents(
                  original_values[i]));
  }
}

TEST_F(UserActivityLoggerDelegateUkmTest, Basic) {
  Browser::CreateParams params(profile(), true);
  auto browser = CreateBrowserWithTestWindowForParams(&params);
  TabStripModel* tab_strip_model = browser->tab_strip_model();
  content::WebContents* fg_contents =
      AddWebContentsAndNavigate(tab_strip_model, kTestUrls[0]);
  tab_strip_model->ActivateTabAt(0, false);
  WebContentsTester::For(fg_contents)->TestSetIsLoading(false);
  const ukm::SourceId source_id0 =
      ukm_entry_checker()->GetSoureIdForUrl(kTestUrls[0]);

  content::WebContents* bg_contents =
      AddWebContentsAndNavigate(tab_strip_model, kTestUrls[1]);
  WebContentsTester::For(bg_contents)->TestSetIsLoading(false);
  const ukm::SourceId source_id1 =
      ukm_entry_checker()->GetSoureIdForUrl(kTestUrls[1]);

  UpdateOpenTabsURLs();
  LogActivity();

  EXPECT_TRUE(
      ukm_entry_checker()->WasNewEntryRecorded(kUserActivityEntryName, 1));
  EXPECT_TRUE(
      ukm_entry_checker()->WasNewEntryRecorded(kUserActivityIdEntryName, 2));

  ukm_entry_checker()->ExpectNewEntry(kUserActivityEntryName, GURL(""),
                                      kUseActivityValues);

  const ukm::mojom::UkmEntry* last_activity_entry =
      ukm_entry_checker()->LastUkmEntry(kUserActivityEntryName);

  // Explicitly check kTimeSinceLastKeyName is not recorded.
  const ukm::mojom::UkmMetric* metric_value = ukm::TestUkmRecorder::FindMetric(
      last_activity_entry, UserActivity::kTimeSinceLastKeyName);

  EXPECT_EQ(nullptr, metric_value);

  const ukm::SourceId source_id = last_activity_entry->source_id;

  const UkmMetricMap kUseActivityIdValues({
      {UserActivityId::kActivityIdName, source_id},
  });

  const std::map<ukm::SourceId, std::pair<GURL, UkmMetricMap>> expected_data({
      {source_id0, std::make_pair(kTestUrls[0], kUseActivityIdValues)},
      {source_id1, std::make_pair(kTestUrls[1], kUseActivityIdValues)},
  });

  ukm_entry_checker()->ExpectNewEntries(kUserActivityIdEntryName,
                                        expected_data);

  tab_strip_model->CloseAllTabs();
}

TEST_F(UserActivityLoggerDelegateUkmTest, NoOpenTabs) {
  Browser::CreateParams params(profile(), true);
  auto browser = CreateBrowserWithTestWindowForParams(&params);

  UpdateOpenTabsURLs();
  LogActivity();

  EXPECT_TRUE(
      ukm_entry_checker()->WasNewEntryRecorded(kUserActivityEntryName, 1));
  EXPECT_TRUE(
      ukm_entry_checker()->WasNewEntryRecorded(kUserActivityIdEntryName, 0));

  ukm_entry_checker()->ExpectNewEntry(kUserActivityEntryName, GURL(""),
                                      kUseActivityValues);

  const ukm::mojom::UkmEntry* last_activity_entry =
      ukm_entry_checker()->LastUkmEntry(kUserActivityEntryName);

  const ukm::mojom::UkmMetric* metric_value = ukm::TestUkmRecorder::FindMetric(
      last_activity_entry, UserActivity::kTimeSinceLastKeyName);

  EXPECT_EQ(nullptr, metric_value);
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
