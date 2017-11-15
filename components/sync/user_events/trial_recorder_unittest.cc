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

void CreateAndFinalizeTrial(const std::string& trial,
                            const std::string& group) {
  // Must actually query for the group to finalize the field trial.
  EXPECT_EQ(group,
            FieldTrialList::CreateFieldTrial(trial, group)->group_name());
  // Notification is posted, which we need the recorder to pick up.
  base::RunLoop().RunUntilIdle();
}

class TrialRecorderTest : public testing::Test {
 public:
  TrialRecorderTest() : field_trial_list_(nullptr), recorder_(&service_) {}

  FakeUserEventService* service() { return &service_; }
  TrialRecorder* recorder() { return &recorder_; }

 private:
  base::MessageLoop message_loop_;
  FieldTrialList field_trial_list_;
  FakeUserEventService service_;
  TrialRecorder recorder_;
};

TEST_F(TrialRecorderTest, OnEventCaseRecordedEdge) {
  CreateAndFinalizeTrial(kTrial1, kGroup);
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial1);
  EXPECT_EQ(0u, service()->GetRecordedUserEvents().size());

  recorder()->OnEventCaseRecorded(UserEventSpecifics::kTestEvent);
  EXPECT_EQ(1u, service()->GetRecordedUserEvents().size());

  // Subsequent records should only record themselves.
  recorder()->OnEventCaseRecorded(UserEventSpecifics::kTestEvent);
  EXPECT_EQ(1u, service()->GetRecordedUserEvents().size());
}

TEST_F(TrialRecorderTest, TrialFinalizedEdge) {
  recorder()->OnEventCaseRecorded(UserEventSpecifics::kTestEvent);
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial1);
  EXPECT_EQ(0u, service()->GetRecordedUserEvents().size());

  CreateAndFinalizeTrial(kTrial1, kGroup);
  EXPECT_EQ(1u, service()->GetRecordedUserEvents().size());

  // Being re-notified of the same finalization should not trigger re-emission.
  recorder()->OnFieldTrialGroupFinalized(kTrial1, kGroup);
  EXPECT_EQ(1u, service()->GetRecordedUserEvents().size());
}

TEST_F(TrialRecorderTest, DependencyRegisteredEdge) {
  recorder()->OnEventCaseRecorded(UserEventSpecifics::kTestEvent);
  CreateAndFinalizeTrial(kTrial1, kGroup);
  EXPECT_EQ(0u, service()->GetRecordedUserEvents().size());

  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial1);
  EXPECT_EQ(1u, service()->GetRecordedUserEvents().size());

  // Re-registering the same dependency shouldn't trigger a re-emission.
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial1);
  EXPECT_EQ(1u, service()->GetRecordedUserEvents().size());
}

TEST_F(TrialRecorderTest, FieldTrialChecksDontFinalize) {
  // We avoid asking for the group, which would finalize. The recorder should be
  // careful to do the same.
  EXPECT_TRUE(FieldTrialList::CreateFieldTrial(kTrial1, kGroup));

  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial1);
  recorder()->OnEventCaseRecorded(UserEventSpecifics::kTestEvent);
  EXPECT_EQ(0u, service()->GetRecordedUserEvents().size());

  // Active and finalized are synonymous from our perspective.
  EXPECT_FALSE(FieldTrialList::IsTrialActive(kTrial1));
}

TEST_F(TrialRecorderTest, FieldTrialBatched) {
  CreateAndFinalizeTrial(kTrial1, kGroup);
  CreateAndFinalizeTrial(kTrial2, kGroup);
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
                   .field_trial_pairs_size());
}

TEST_F(TrialRecorderTest, MultipleFieldTrials) {
  recorder()->OnEventCaseRecorded(UserEventSpecifics::kTestEvent);
  recorder()->OnEventCaseRecorded(UserEventSpecifics::kLanguageDetectionEvent);

  // Both of these will be satisfied, should emit.
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial1);
  recorder()->RegisterDependentFieldTrial(
      UserEventSpecifics::kLanguageDetectionEvent, kTrial1);
  CreateAndFinalizeTrial(kTrial1, kGroup);
  EXPECT_EQ(1u, service()->GetRecordedUserEvents().size());

  // One of these will be satisfied, should emit.
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial2);
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTranslationEvent,
                                          kTrial2);
  CreateAndFinalizeTrial(kTrial2, kGroup);
  EXPECT_EQ(2u, service()->GetRecordedUserEvents().size());

  // Neither will be satisfied, should not emit.
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTranslationEvent,
                                          kTrial3);
  recorder()->RegisterDependentFieldTrial(
      UserEventSpecifics::kGaiaPasswordReuseEvent, kTrial3);
  CreateAndFinalizeTrial(kTrial3, kGroup);
  EXPECT_EQ(2u, service()->GetRecordedUserEvents().size());

  // The second FieldTrial event should contain both trials.
  EXPECT_EQ(2, service()
                   ->GetRecordedUserEvents()
                   .rbegin()
                   ->field_trial_event()
                   .field_trial_pairs_size());
}

TEST_F(TrialRecorderTest, FieldTrialFeatureDisabled) {
  ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(
      switches::kSyncUserFieldTrialEvents);

  EXPECT_TRUE(FieldTrialList::CreateFieldTrial(kTrial1, kGroup));
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial1);
  recorder()->OnEventCaseRecorded(UserEventSpecifics::kTestEvent);
  EXPECT_EQ(0u, service()->GetRecordedUserEvents().size());
}

TEST_F(TrialRecorderTest, FieldTrialOverMax) {
  CreateAndFinalizeTrial(kTrial1, kGroup);
  CreateAndFinalizeTrial(kTrial2, kGroup);
  ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      switches::kSyncUserFieldTrialEvents, {{"field_trial_max_count", "1"}});
  recorder()->OnEventCaseRecorded(UserEventSpecifics::kTestEvent);

  // |kTrial1| will the be the first trial, and thus should be recorded.
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial1);
  EXPECT_EQ(1u, service()->GetRecordedUserEvents().size());

  // Recording |kTrial2| would bring the count of trials over the max, so it
  // should be skipped instead.
  recorder()->RegisterDependentFieldTrial(UserEventSpecifics::kTestEvent,
                                          kTrial2);
  EXPECT_EQ(1u, service()->GetRecordedUserEvents().size());
}

TEST_F(TrialRecorderTest, FieldTrialTimer) {
  CreateAndFinalizeTrial(kTrial1, kGroup);

  {
    // Start with 0 delay, which should mean we post immediately to record.
    // Need to be not call any methods that might invoke RunUntilIdle() while we
    // have a delay of 0, like CreateAndFinalizeTrial().
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

}  // namespace

}  // namespace syncer
