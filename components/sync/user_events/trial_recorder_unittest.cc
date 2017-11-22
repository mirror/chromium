// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/user_events/trial_recorder.h"

#include "base/message_loop/message_loop.h"
#include "base/metrics/field_trial.h"
#include "base/test/scoped_feature_list.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync/user_events/fake_user_event_service.h"
#include "components/variations/variations_associated_data.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::FieldTrialList;
using base::test::ScopedFeatureList;
using sync_pb::UserEventSpecifics;

namespace syncer {

namespace {

const char kTrial1[] = "TrialNameOne";
const char kTrial2[] = "TrialNameTwo";
const char kTrial3[] = "TrialNameThree";
const char kGroup[] = "GroupName";
const variations::VariationID kId1 = 111;
const variations::VariationID kId2 = 222;
const variations::VariationID kId3 = 333;

void SetupTrial(const std::string& trial,
                variations::VariationID variation_id) {
  FieldTrialList::CreateFieldTrial(trial, kGroup);
  variations::AssociateGoogleVariationID(variations::CHROME_SYNC_SERVICE, trial,
                                         kGroup, variation_id);
}

void VerifyEvent(std::set<variations::VariationID> expected_variations,
                 const UserEventSpecifics& actual) {
  ASSERT_EQ(
      expected_variations.size(),
      static_cast<size_t>(actual.field_trial_event().variation_ids_size()));
  for (int actaul_variation : actual.field_trial_event().variation_ids()) {
    auto iter = expected_variations.find(actaul_variation);
    if (iter == expected_variations.end()) {
      FAIL() << actaul_variation;
    } else {
      // Remove to make sure the event doesn't contain duplicates.
      expected_variations.erase(iter);
    }
  }
}

class TrialRecorderTest : public testing::Test {
 public:
  TrialRecorderTest() : field_trial_list_(nullptr), recorder_(&service_) {}

  ~TrialRecorderTest() override { variations::testing::ClearAllVariationIDs(); }

  FakeUserEventService* service() { return &service_; }
  TrialRecorder* recorder() { return &recorder_; }

  void VerifyLastEvent(std::set<variations::VariationID> expected_variations) {
    ASSERT_LE(1u, service()->GetRecordedUserEvents().size());
    VerifyEvent(expected_variations,
                *service()->GetRecordedUserEvents().rbegin());
  }

 private:
  base::MessageLoop message_loop_;
  FieldTrialList field_trial_list_;
  FakeUserEventService service_;
  TrialRecorder recorder_;
};

TEST_F(TrialRecorderTest, RegisterThenOnEventCase) {
  SetupTrial(kTrial1, kId1);

  // RegisterDependentFieldTrial is necessary but not sufficient.
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial1);
  EXPECT_EQ(0u, service()->GetRecordedUserEvents().size());

  // RegisterDependentFieldTrial will record a FieldTrial event.
  recorder()->OnEventCaseRecorded(UserEventSpecifics::kTestEvent);
  EXPECT_EQ(1u, service()->GetRecordedUserEvents().size());
  VerifyLastEvent({kId1});

  // Subsequent records should only record themselves.
  recorder()->OnEventCaseRecorded(UserEventSpecifics::kTestEvent);
  EXPECT_EQ(1u, service()->GetRecordedUserEvents().size());
}

// Same as RegisterThenOnEventCase but with a different sequence of events.
TEST_F(TrialRecorderTest, DependencyRegisteredEdge) {
  SetupTrial(kTrial1, kId1);

  // OnEventCaseRecorded is necessary but not sufficient.
  recorder()->OnEventCaseRecorded(UserEventSpecifics::kTestEvent);
  EXPECT_EQ(0u, service()->GetRecordedUserEvents().size());

  // RegisterDependentFieldTrial will record a FieldTrial event.
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial1);
  EXPECT_EQ(1u, service()->GetRecordedUserEvents().size());
  VerifyLastEvent({kId1});

  // Re-registering the same dependency shouldn't trigger a re-emission.
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial1);
  EXPECT_EQ(1u, service()->GetRecordedUserEvents().size());
}

TEST_F(TrialRecorderTest, FieldTrialBatched) {
  SetupTrial(kTrial1, kId1);
  SetupTrial(kTrial2, kId2);
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial1);
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial2);

  // The two trials should both be included in a single FieldTrial event.
  recorder()->OnEventCaseRecorded(UserEventSpecifics::kTestEvent);
  ASSERT_EQ(1u, service()->GetRecordedUserEvents().size());
  EXPECT_EQ(2, service()
                   ->GetRecordedUserEvents()
                   .begin()
                   ->field_trial_event()
                   .variation_ids_size());
  VerifyLastEvent({kId1, kId2});
}

TEST_F(TrialRecorderTest, MultipleFieldTrials) {
  SetupTrial(kTrial1, kId1);
  SetupTrial(kTrial2, kId2);
  SetupTrial(kTrial3, kId3);
  recorder()->OnEventCaseRecorded(UserEventSpecifics::kTestEvent);
  recorder()->OnEventCaseRecorded(UserEventSpecifics::kLanguageDetectionEvent);

  // Both of these will be satisfied, should emit.
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial1);
  recorder()->RegisterDependentFieldTrial(
      UserEventSpecifics::kLanguageDetectionEvent, kTrial1);
  EXPECT_EQ(1u, service()->GetRecordedUserEvents().size());
  VerifyLastEvent({kId1});

  // One of these will be satisfied, should emit.
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial2);
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTranslationEvent,
                                          kTrial2);
  EXPECT_EQ(2u, service()->GetRecordedUserEvents().size());
  VerifyLastEvent({kId1, kId2});

  // Neither will be satisfied, should not emit.
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTranslationEvent,
                                          kTrial3);
  recorder()->RegisterDependentFieldTrial(
      UserEventSpecifics::kGaiaPasswordReuseEvent, kTrial3);
  EXPECT_EQ(2u, service()->GetRecordedUserEvents().size());

  // The second FieldTrial event should contain both trials.
  EXPECT_EQ(2, service()
                   ->GetRecordedUserEvents()
                   .rbegin()
                   ->field_trial_event()
                   .variation_ids_size());
}

TEST_F(TrialRecorderTest, FieldTrialFeatureDisabled) {
  ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      switches::kSyncUserFieldTrialEvents);
  SetupTrial(kTrial1, kId1);
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial1);
  recorder()->OnEventCaseRecorded(UserEventSpecifics::kTestEvent);
  EXPECT_EQ(0u, service()->GetRecordedUserEvents().size());
}

TEST_F(TrialRecorderTest, FieldTrialTimer) {
  SetupTrial(kTrial1, kId1);

  {
    // Start with 0 delay, which should mean we post immediately to record.
    // Need to be not call any methods that might invoke RunUntilIdle() while we
    // have a delay of 0, like SetupTrial().
    ScopedFeatureList scoped_feature_list;
    scoped_feature_list.InitAndEnableFeatureWithParameters(
        switches::kSyncUserFieldTrialEvents,
        {{"field_trial_delay_seconds", "0"}});

    recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                            kTrial1);
    recorder()->OnEventCaseRecorded(UserEventSpecifics::kTestEvent);
    EXPECT_EQ(1u, service()->GetRecordedUserEvents().size());
  }

  // Now that |scoped_feature_list| is gone, we should reset to default,
  // otherwise our RunUntilIdle() would infinitively loop with a delay of 0.
  base::RunLoop().RunUntilIdle();
  // Should have picked up exactly 1 more event.
  EXPECT_EQ(2u, service()->GetRecordedUserEvents().size());
}

TEST_F(TrialRecorderTest, NoVariationId) {
  FieldTrialList::CreateFieldTrial(kTrial1, kGroup);
  recorder()->OnEventCaseRecorded(UserEventSpecifics::kTestEvent);
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial1);
  // No variation id was ever specified, so nothing should be recorded.
  EXPECT_EQ(0u, service()->GetRecordedUserEvents().size());
}

TEST_F(TrialRecorderTest, NonSyncVariationId) {
  FieldTrialList::CreateFieldTrial(kTrial1, kGroup);
  // The collection id is not CHROME_SYNC_SERVICE, and thus should be ignored.
  variations::AssociateGoogleVariationID(variations::GOOGLE_WEB_PROPERTIES,
                                         kTrial1, kGroup, kId1);
  recorder()->OnEventCaseRecorded(UserEventSpecifics::kTestEvent);
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial1);
  EXPECT_EQ(0u, service()->GetRecordedUserEvents().size());
}

}  // namespace

}  // namespace syncer
