// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <vector>

#include "components/prefs/testing_pref_service.h"
#include "components/update_client/persisted_data.h"
#include "extensions/features/features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace update_client {

TEST(PersistedDataTest, Simple) {
  std::unique_ptr<TestingPrefServiceSimple> pref(
      new TestingPrefServiceSimple());
  PersistedData::RegisterPrefs(pref->registry());
  std::unique_ptr<PersistedData> metadata(new PersistedData(pref.get()));
  EXPECT_EQ(-2, metadata->GetDateLastRollCall("someappid"));

#if BUILDFLAG(ENABLE_EXTENSIONS)
  EXPECT_EQ(-2, metadata->GetDaysSinceLastRollCall("someappid"));
  EXPECT_EQ(-2, metadata->GetDateLastActive("someappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastActive("someappid"));
  EXPECT_FALSE(metadata->GetActiveBit("someappid"));
  EXPECT_FALSE(metadata->GetActiveBit("someotherappid"));

  // Days since last roll call/active.
  metadata->SetLastRollCallTime("someappid", base::Time::Now());
  metadata->SetLastActiveTime("someappid", base::Time::Now());
  EXPECT_EQ(0, metadata->GetDaysSinceLastRollCall("someappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastActive("someappid"));

  metadata->SetLastRollCallTime(
      "someappid", base::Time::Now() - base::TimeDelta::FromDays(2));
  EXPECT_EQ(2, metadata->GetDaysSinceLastRollCall("someappid"));

  // When active bit is not set, active days/date will not change.
  metadata->SetDateLastActive({"someappid"}, 3383);
  metadata->SetDateLastActive({"someotherappid"}, 3383);
  EXPECT_EQ(-2, metadata->GetDateLastActive("someappid"));
  EXPECT_EQ(-2, metadata->GetDateLastActive("someotherappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastActive("someappid"));
  EXPECT_EQ(2, metadata->GetDaysSinceLastRollCall("someappid"));

  metadata->SetActiveBit("someappid");
  EXPECT_TRUE(metadata->GetActiveBit("someappid"));
  EXPECT_FALSE(metadata->GetActiveBit("someotherappid"));
  metadata->SetDateLastActive({"someappid"}, 3383);
  metadata->SetDateLastActive({"someotherappid"}, 3383);
  EXPECT_EQ(3383, metadata->GetDateLastActive("someappid"));
  EXPECT_EQ(-2, metadata->GetDateLastActive("someotherappid"));
  EXPECT_EQ(2, metadata->GetDaysSinceLastRollCall("someappid"));
  EXPECT_EQ(-2, metadata->GetDaysSinceLastActive("someappid"));
  metadata->SetLastActiveTime("someappid",
                              base::Time::Now() - base::TimeDelta::FromDays(3));
  EXPECT_EQ(3, metadata->GetDaysSinceLastActive("someappid"));

  metadata->ClearActiveBit({"someappid", "someotherappid"});
  EXPECT_FALSE(metadata->GetActiveBit("someappid"));
  EXPECT_FALSE(metadata->GetActiveBit("someotherappid"));
  metadata->SetDateLastActive({"someappid"}, 3385);
  metadata->SetDateLastActive({"someotherappid"}, 3385);
  EXPECT_EQ(3383, metadata->GetDateLastActive("someappid"));
  EXPECT_EQ(-2, metadata->GetDateLastActive("someotherappid"));
  EXPECT_EQ(2, metadata->GetDaysSinceLastRollCall("someappid"));
  EXPECT_EQ(3, metadata->GetDaysSinceLastActive("someappid"));
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

  std::vector<std::string> items;
  items.push_back("someappid");
  metadata->SetDateLastRollCall(items, 3383);
  EXPECT_EQ(3383, metadata->GetDateLastRollCall("someappid"));
  EXPECT_EQ(-2, metadata->GetDateLastRollCall("someotherappid"));
  const std::string pf1 = metadata->GetPingFreshness("someappid");
  EXPECT_FALSE(pf1.empty());
  metadata->SetDateLastRollCall(items, 3386);
  EXPECT_EQ(3386, metadata->GetDateLastRollCall("someappid"));
  EXPECT_EQ(-2, metadata->GetDateLastRollCall("someotherappid"));
  const std::string pf2 = metadata->GetPingFreshness("someappid");
  EXPECT_FALSE(pf2.empty());
  // The following has a 1 / 2^128 chance of being flaky.
  EXPECT_NE(pf1, pf2);
}

TEST(PersistedDataTest, SharedPref) {
  std::unique_ptr<TestingPrefServiceSimple> pref(
      new TestingPrefServiceSimple());
  PersistedData::RegisterPrefs(pref->registry());
  std::unique_ptr<PersistedData> metadata(new PersistedData(pref.get()));
  EXPECT_EQ(-2, metadata->GetDateLastRollCall("someappid"));
  std::vector<std::string> items;
  items.push_back("someappid");
  metadata->SetDateLastRollCall(items, 3383);

  // Now, create a new PersistedData reading from the same path, verify
  // that it loads the value.
  metadata.reset(new PersistedData(pref.get()));
  EXPECT_EQ(3383, metadata->GetDateLastRollCall("someappid"));
  EXPECT_EQ(-2, metadata->GetDateLastRollCall("someotherappid"));
}

TEST(PersistedDataTest, SimpleCohort) {
  std::unique_ptr<TestingPrefServiceSimple> pref(
      new TestingPrefServiceSimple());
  PersistedData::RegisterPrefs(pref->registry());
  std::unique_ptr<PersistedData> metadata(new PersistedData(pref.get()));
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

}  // namespace update_client
