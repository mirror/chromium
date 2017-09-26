// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/data_use_tracker.h"

#include "base/strings/stringprintf.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace metrics {

namespace {

const char kTodayStr[] = "2016-03-16";
const char kYesterdayStr[] = "2016-03-15";
const char kExpiredDateStr1[] = "2016-03-09";
const char kExpiredDateStr2[] = "2016-03-01";

class TestDataUsePrefService : public TestingPrefServiceSimple {
 public:
  TestDataUsePrefService() { DataUseTracker::RegisterPrefs(registry()); }

  void ClearDataUsePrefs() {
    ClearPref(metrics::prefs::kUserCellDataUse);
    ClearPref(metrics::prefs::kUmaCellDataUse);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestDataUsePrefService);
};

class FakeDataUseTracker : public DataUseTracker {
 public:
  FakeDataUseTracker(PrefService* local_state) : DataUseTracker(local_state) {}

  bool GetUmaWeeklyQuota(int* uma_weekly_quota_bytes) const override {
    *uma_weekly_quota_bytes = 200;
    return true;
  }

  bool GetUmaRatio(double* ratio) const override {
    *ratio = 0.05;
    return true;
  }

  base::Time GetCurrentMeasurementDate() const override {
    base::Time today_for_test;
    EXPECT_TRUE(base::Time::FromUTCString(kTodayStr, &today_for_test));
    return today_for_test;
  }

  std::string GetCurrentMeasurementDateAsString() const override {
    return kTodayStr;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeDataUseTracker);
};

// Sets up data usage prefs with mock values so that UMA traffic is above the
// allowed ratio.
void SetPrefTestValuesOverRatio(PrefService* local_state) {
  base::DictionaryValue user_pref_dict;
  user_pref_dict.SetInteger(kTodayStr, 2 * 100);
  user_pref_dict.SetInteger(kYesterdayStr, 2 * 100);
  user_pref_dict.SetInteger(kExpiredDateStr1, 2 * 100);
  user_pref_dict.SetInteger(kExpiredDateStr2, 2 * 100);
  local_state->Set(prefs::kUserCellDataUse, user_pref_dict);

  base::DictionaryValue uma_pref_dict;
  uma_pref_dict.SetInteger(kTodayStr, 50);
  uma_pref_dict.SetInteger(kYesterdayStr, 50);
  uma_pref_dict.SetInteger(kExpiredDateStr1, 50);
  uma_pref_dict.SetInteger(kExpiredDateStr2, 50);
  local_state->Set(prefs::kUmaCellDataUse, uma_pref_dict);
}

}  // namespace

TEST(DataUseTrackerTest, CheckUpdateUsagePref) {
  TestDataUsePrefService local_state;
  FakeDataUseTracker data_use_tracker(&local_state);
  local_state.ClearDataUsePrefs();

  int user_pref_value = 0;
  int uma_pref_value = 0;

  data_use_tracker.UpdateMetricsUsagePrefs("", 2 * 100, true);
  local_state.GetDictionary(prefs::kUserCellDataUse)
      ->GetInteger(kTodayStr, &user_pref_value);
  EXPECT_EQ(2 * 100, user_pref_value);
  local_state.GetDictionary(prefs::kUmaCellDataUse)
      ->GetInteger(kTodayStr, &uma_pref_value);
  EXPECT_EQ(0, uma_pref_value);

  data_use_tracker.UpdateMetricsUsagePrefs("UMA", 100, true);
  local_state.GetDictionary(prefs::kUserCellDataUse)
      ->GetInteger(kTodayStr, &user_pref_value);
  EXPECT_EQ(3 * 100, user_pref_value);
  local_state.GetDictionary(prefs::kUmaCellDataUse)
      ->GetInteger(kTodayStr, &uma_pref_value);
  EXPECT_EQ(100, uma_pref_value);
}

TEST(DataUseTrackerTest, CheckRemoveExpiredEntries) {
  TestDataUsePrefService local_state;
  FakeDataUseTracker data_use_tracker(&local_state);
  local_state.ClearDataUsePrefs();
  SetPrefTestValuesOverRatio(&local_state);
  data_use_tracker.RemoveExpiredEntries();

  int user_pref_value = 0;
  int uma_pref_value = 0;

  local_state.GetDictionary(prefs::kUserCellDataUse)
      ->GetInteger(kExpiredDateStr1, &user_pref_value);
  EXPECT_EQ(0, user_pref_value);
  local_state.GetDictionary(prefs::kUmaCellDataUse)
      ->GetInteger(kExpiredDateStr1, &uma_pref_value);
  EXPECT_EQ(0, uma_pref_value);

  local_state.GetDictionary(prefs::kUserCellDataUse)
      ->GetInteger(kExpiredDateStr2, &user_pref_value);
  EXPECT_EQ(0, user_pref_value);
  local_state.GetDictionary(prefs::kUmaCellDataUse)
      ->GetInteger(kExpiredDateStr2, &uma_pref_value);
  EXPECT_EQ(0, uma_pref_value);

  local_state.GetDictionary(prefs::kUserCellDataUse)
      ->GetInteger(kTodayStr, &user_pref_value);
  EXPECT_EQ(2 * 100, user_pref_value);
  local_state.GetDictionary(prefs::kUmaCellDataUse)
      ->GetInteger(kTodayStr, &uma_pref_value);
  EXPECT_EQ(50, uma_pref_value);

  local_state.GetDictionary(prefs::kUserCellDataUse)
      ->GetInteger(kYesterdayStr, &user_pref_value);
  EXPECT_EQ(2 * 100, user_pref_value);
  local_state.GetDictionary(prefs::kUmaCellDataUse)
      ->GetInteger(kYesterdayStr, &uma_pref_value);
  EXPECT_EQ(50, uma_pref_value);
}

TEST(DataUseTrackerTest, CheckComputeTotalDataUse) {
  TestDataUsePrefService local_state;
  FakeDataUseTracker data_use_tracker(&local_state);
  local_state.ClearDataUsePrefs();
  SetPrefTestValuesOverRatio(&local_state);

  int user_data_use =
      data_use_tracker.ComputeTotalDataUse(prefs::kUserCellDataUse);
  EXPECT_EQ(8 * 100, user_data_use);
  int uma_data_use =
      data_use_tracker.ComputeTotalDataUse(prefs::kUmaCellDataUse);
  EXPECT_EQ(4 * 50, uma_data_use);
}

TEST(DataUseTrackerTest, GetUploadReductionPercent) {
  TestDataUsePrefService local_state;
  FakeDataUseTracker data_use_tracker(&local_state);
  local_state.ClearDataUsePrefs();
  SetPrefTestValuesOverRatio(&local_state);

  // Test values are at 100 of 200 weekly quota.
  EXPECT_EQ(0, data_use_tracker.GetUploadReductionPercent());

  // Another 100 brings to the edge of weekly quota.
  data_use_tracker.UpdateMetricsUsagePrefs("UMA", 100, true);
  EXPECT_EQ(0, data_use_tracker.GetUploadReductionPercent());

  // Well over the ratio (40% vs 5%) so full reduction.
  data_use_tracker.UpdateMetricsUsagePrefs("UKM", 1, true);
  EXPECT_EQ(90, data_use_tracker.GetUploadReductionPercent());

  // Record other data so about 4% (30% between 2.5% to 7.5%).
  data_use_tracker.UpdateMetricsUsagePrefs("foo", 4500, true);
  EXPECT_EQ(30, data_use_tracker.GetUploadReductionPercent());

  // Record other data so about 2%.
  data_use_tracker.UpdateMetricsUsagePrefs("foo", 5000, true);
  EXPECT_EQ(0, data_use_tracker.GetUploadReductionPercent());

  // Bring UMA data up to 2.5%.
  data_use_tracker.UpdateMetricsUsagePrefs("UMA", 51, true);
  EXPECT_EQ(0, data_use_tracker.GetUploadReductionPercent());

  // Bring UMA data up to 6.5% (80% between 2.5% to 7.5%).
  data_use_tracker.UpdateMetricsUsagePrefs("UKM", 430, true);
  EXPECT_EQ(80, data_use_tracker.GetUploadReductionPercent());

  // cellular=false, no limits.
  data_use_tracker.UpdateMetricsUsagePrefs("UMA", 1, false);
  EXPECT_EQ(0, data_use_tracker.GetUploadReductionPercent());
}

}  // namespace metrics
