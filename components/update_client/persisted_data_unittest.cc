// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "components/prefs/testing_pref_service.h"
#include "components/update_client/activity_data_service.h"
#include "components/update_client/persisted_data.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace update_client {

TEST(PersistedDataTest, Simple) {
  std::unique_ptr<TestingPrefServiceSimple> pref =
      std::make_unique<TestingPrefServiceSimple>();
  PersistedData::RegisterPrefs(pref->registry());
  std::unique_ptr<PersistedData> metadata =
      std::make_unique<PersistedData>(pref.get(), nullptr);
  EXPECT_EQ(-2, metadata->GetDateLastRollCall("someappid"));
  EXPECT_EQ(-2, metadata->GetDateLastActive("someappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastRollCall("someappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastActive("someappid"));
  std::vector<std::string> items;
  items.push_back("someappid");
  metadata->SetDateLastRollCall(items, 3383);
  metadata->SetDateLastActive(items, 3383);
  EXPECT_EQ(3383, metadata->GetDateLastRollCall("someappid"));
  EXPECT_EQ(-2, metadata->GetDateLastActive("someappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastRollCall("someappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastActive("someappid"));
  EXPECT_EQ(-2, metadata->GetDateLastRollCall("someotherappid"));
  EXPECT_EQ(-2, metadata->GetDateLastActive("someotherappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastRollCall("someotherappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastActive("someotherappid"));
  const std::string pf1 = metadata->GetPingFreshness("someappid");
  EXPECT_FALSE(pf1.empty());
  metadata->SetDateLastRollCall(items, 3386);
  metadata->SetDateLastActive(items, 3386);
  EXPECT_EQ(3386, metadata->GetDateLastRollCall("someappid"));
  EXPECT_EQ(-2, metadata->GetDateLastActive("someappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastRollCall("someappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastActive("someappid"));
  EXPECT_EQ(-2, metadata->GetDateLastRollCall("someotherappid"));
  EXPECT_EQ(-2, metadata->GetDateLastActive("someotherappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastRollCall("someotherappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastActive("someotherappid"));
  const std::string pf2 = metadata->GetPingFreshness("someappid");
  EXPECT_FALSE(pf2.empty());
  // The following has a 1 / 2^128 chance of being flaky.
  EXPECT_NE(pf1, pf2);
}

TEST(PersistedDataTest, SharedPref) {
  std::unique_ptr<TestingPrefServiceSimple> pref =
      std::make_unique<TestingPrefServiceSimple>();
  PersistedData::RegisterPrefs(pref->registry());
  std::unique_ptr<PersistedData> metadata =
      std::make_unique<PersistedData>(pref.get(), nullptr);
  EXPECT_EQ(-2, metadata->GetDateLastRollCall("someappid"));
  EXPECT_EQ(-2, metadata->GetDateLastActive("someappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastRollCall("someappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastActive("someappid"));
  std::vector<std::string> items;
  items.push_back("someappid");
  metadata->SetDateLastRollCall(items, 3383);
  metadata->SetDateLastActive(items, 3383);

  // Now, create a new PersistedData reading from the same path, verify
  // that it loads the value.
  metadata = std::make_unique<PersistedData>(pref.get(), nullptr);
  EXPECT_EQ(3383, metadata->GetDateLastRollCall("someappid"));
  EXPECT_EQ(-2, metadata->GetDateLastActive("someappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastRollCall("someappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastActive("someappid"));
  EXPECT_EQ(-2, metadata->GetDateLastRollCall("someotherappid"));
  EXPECT_EQ(-2, metadata->GetDateLastActive("someotherappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastRollCall("someotherappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastActive("someotherappid"));
}

TEST(PersistedDataTest, SimpleCohort) {
  std::unique_ptr<TestingPrefServiceSimple> pref =
      std::make_unique<TestingPrefServiceSimple>();
  PersistedData::RegisterPrefs(pref->registry());
  std::unique_ptr<PersistedData> metadata =
      std::make_unique<PersistedData>(pref.get(), nullptr);
  EXPECT_EQ("", metadata->GetCohort("someappid"));
  EXPECT_EQ("", metadata->GetCohort("someotherappid"));
  EXPECT_EQ("", metadata->GetCohortHint("someappid"));
  EXPECT_EQ("", metadata->GetCohortHint("someotherappid"));
  EXPECT_EQ("", metadata->GetCohortName("someappid"));
  EXPECT_EQ("", metadata->GetCohortName("someotherappid"));
  metadata->SetCohort("someappid", "c1");
  metadata->SetCohort("someotherappid", "c2");
  metadata->SetCohortHint("someappid", "ch1");
  metadata->SetCohortHint("someotherappid", "ch2");
  metadata->SetCohortName("someappid", "cn1");
  metadata->SetCohortName("someotherappid", "cn2");
  EXPECT_EQ("c1", metadata->GetCohort("someappid"));
  EXPECT_EQ("c2", metadata->GetCohort("someotherappid"));
  EXPECT_EQ("ch1", metadata->GetCohortHint("someappid"));
  EXPECT_EQ("ch2", metadata->GetCohortHint("someotherappid"));
  EXPECT_EQ("cn1", metadata->GetCohortName("someappid"));
  EXPECT_EQ("cn2", metadata->GetCohortName("someotherappid"));
  metadata->SetCohort("someappid", "oc1");
  metadata->SetCohort("someotherappid", "oc2");
  metadata->SetCohortHint("someappid", "och1");
  metadata->SetCohortHint("someotherappid", "och2");
  metadata->SetCohortName("someappid", "ocn1");
  metadata->SetCohortName("someotherappid", "ocn2");
  EXPECT_EQ("oc1", metadata->GetCohort("someappid"));
  EXPECT_EQ("oc2", metadata->GetCohort("someotherappid"));
  EXPECT_EQ("och1", metadata->GetCohortHint("someappid"));
  EXPECT_EQ("och2", metadata->GetCohortHint("someotherappid"));
  EXPECT_EQ("ocn1", metadata->GetCohortName("someappid"));
  EXPECT_EQ("ocn2", metadata->GetCohortName("someotherappid"));
}

TEST(PersistedDataTest, ActivityData) {
  class MockActivityDataService : public ActivityDataService {
   public:
    bool GetActiveBit(const std::string& id) const override {
      auto it = active_.find(id);
      if (it != active_.end()) {
        return it->second;
      }
      return false;
    }

    int GetDaysSinceLastActive(const std::string& id) const override {
      auto it = days_last_active_.find(id);
      if (it != days_last_active_.end()) {
        return it->second;
      }
      return -1;
    }

    int GetDaysSinceLastRollCall(const std::string& id) const override {
      auto it = days_last_roll_call_.find(id);
      if (it != days_last_roll_call_.end()) {
        return it->second;
      }
      return -1;
    }

    void ClearActiveBit(const std::string& id) override { active_[id] = false; }

    void SetDaysSinceLastActive(const std::string& id, int numdays) {
      days_last_active_[id] = numdays;
    }

    void SetDaysSinceLastRollCall(const std::string& id, int numdays) {
      days_last_roll_call_[id] = numdays;
    }

    void SetActiveBit(const std::string& id) { active_[id] = true; }

   private:
    std::map<std::string, bool> active_;
    std::map<std::string, int> days_last_active_;
    std::map<std::string, int> days_last_roll_call_;
  };

  std::unique_ptr<TestingPrefServiceSimple> pref =
      std::make_unique<TestingPrefServiceSimple>();
  std::unique_ptr<MockActivityDataService> activity_service =
      std::make_unique<MockActivityDataService>();
  PersistedData::RegisterPrefs(pref->registry());
  std::unique_ptr<PersistedData> metadata =
      std::make_unique<PersistedData>(pref.get(), activity_service.get());

  EXPECT_EQ(-2, metadata->GetDateLastActive("someappid"));
  EXPECT_EQ(-2, metadata->GetDateLastRollCall("someappid"));
  EXPECT_EQ(-1, metadata->GetDaysSinceLastActive("someappid"));
  EXPECT_EQ(-1, metadata->GetDaysSinceLastRollCall("someappid"));
  EXPECT_EQ(false, metadata->GetActiveBit("someappid"));

  std::vector<std::string> items({"someappid"});

  metadata->SetDateLastActive(items, 1234);
  metadata->SetDateLastRollCall(items, 1234);
  EXPECT_EQ(false, metadata->GetActiveBit("someappid"));
  EXPECT_EQ(-2, metadata->GetDateLastActive("someappid"));
  EXPECT_EQ(1234, metadata->GetDateLastRollCall("someappid"));
  EXPECT_EQ(-1, metadata->GetDaysSinceLastActive("someappid"));
  EXPECT_EQ(-1, metadata->GetDaysSinceLastRollCall("someappid"));

  activity_service->SetActiveBit("someappid");
  activity_service->SetDaysSinceLastActive("someappid", 3);
  activity_service->SetDaysSinceLastRollCall("someappid", 2);
  EXPECT_EQ(true, metadata->GetActiveBit("someappid"));
  EXPECT_EQ(3, metadata->GetDaysSinceLastActive("someappid"));
  EXPECT_EQ(2, metadata->GetDaysSinceLastRollCall("someappid"));

  metadata->SetDateLastActive(items, 3384);
  metadata->SetDateLastRollCall(items, 3383);
  EXPECT_EQ(false, metadata->GetActiveBit("someappid"));
  EXPECT_EQ(3, metadata->GetDaysSinceLastActive("someappid"));
  EXPECT_EQ(2, metadata->GetDaysSinceLastRollCall("someappid"));
  EXPECT_EQ(3384, metadata->GetDateLastActive("someappid"));
  EXPECT_EQ(3383, metadata->GetDateLastRollCall("someappid"));

  metadata->SetDateLastActive(items, 5000);
  metadata->SetDateLastRollCall(items, 3390);
  EXPECT_EQ(false, metadata->GetActiveBit("someappid"));
  EXPECT_EQ(3, metadata->GetDaysSinceLastActive("someappid"));
  EXPECT_EQ(2, metadata->GetDaysSinceLastRollCall("someappid"));
  EXPECT_EQ(3384, metadata->GetDateLastActive("someappid"));
  EXPECT_EQ(3390, metadata->GetDateLastRollCall("someappid"));

  activity_service->SetActiveBit("someappid");
  metadata->SetDateLastActive(items, 5678);
  metadata->SetDateLastRollCall(items, 6789);
  EXPECT_EQ(false, metadata->GetActiveBit("someappid"));
  EXPECT_EQ(3, metadata->GetDaysSinceLastActive("someappid"));
  EXPECT_EQ(2, metadata->GetDaysSinceLastRollCall("someappid"));
  EXPECT_EQ(5678, metadata->GetDateLastActive("someappid"));
  EXPECT_EQ(6789, metadata->GetDateLastRollCall("someappid"));
}

}  // namespace update_client
