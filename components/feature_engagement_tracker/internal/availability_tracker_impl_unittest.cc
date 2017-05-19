// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feature_engagement_tracker/internal/availability_tracker_impl.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/optional.h"
#include "components/feature_engagement_tracker/internal/availability_store.h"
#include "components/feature_engagement_tracker/public/feature_list.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::SaveArg;

namespace feature_engagement_tracker {

namespace {
const base::Feature kTestFeatureFoo{"test_foo",
                                    base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kTestFeatureBar{"test_bar",
                                    base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kTestFeatureQux{"test_qux",
                                    base::FEATURE_DISABLED_BY_DEFAULT};
const base::Feature kTestFeatureNop{"test_nop",
                                    base::FEATURE_DISABLED_BY_DEFAULT};

class MockAvailabilityStore : public AvailabilityStore {
 public:
  explicit MockAvailabilityStore(base::Optional<bool>* destructed)
      : destructed_(destructed) {}

  ~MockAvailabilityStore() override { *destructed_ = true; }

  // AvailabilityStore implementation.
  MOCK_METHOD1(Load, void(const OnLoadedCallback&));

 private:
  base::Optional<bool>* destructed_;

  DISALLOW_COPY_AND_ASSIGN(MockAvailabilityStore);
};

class AvailabilityTrackerImplTest : public testing::Test {
 public:
  AvailabilityTrackerImplTest() {
    load_callback_ = base::Bind(&AvailabilityTrackerImplTest::OnTrackerLoaded,
                                base::Unretained(this));
  }

  ~AvailabilityTrackerImplTest() override = default;

  // SetUpTracker exists so that the filter can be changed for any test.
  void SetUpTracker(std::vector<const base::Feature*> feature_filter) {
    auto mocked_store =
        base::MakeUnique<MockAvailabilityStore>(&store_destructed_);
    mocked_store_ = mocked_store.get();
    tracker_ = base::MakeUnique<AvailabilityTrackerImpl>(
        std::move(mocked_store),
        base::MakeUnique<FeatureVector>(feature_filter));
  }

  void OnTrackerLoaded(bool success) { load_success_ = success; }

 protected:
  MockAvailabilityStore* mocked_store_;
  // Tracks whether the store has been destructed.
  base::Optional<bool> store_destructed_;

  std::unique_ptr<AvailabilityTrackerImpl> tracker_;

  // Load callback tracking.
  base::Optional<bool> load_success_;
  AvailabilityTrackerImpl::OnLoadedCallback load_callback_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AvailabilityTrackerImplTest);
};

}  // namespace

TEST_F(AvailabilityTrackerImplTest, LoadTwoOfThreeFeatures) {
  FeatureVector features;
  features.push_back(&kTestFeatureFoo);  // In DB.
  features.push_back(&kTestFeatureBar);  // In DB.
  features.push_back(&kTestFeatureNop);  // Not in DB. Should become 0u.
  SetUpTracker(features);

  AvailabilityStore::OnLoadedCallback callback;
  EXPECT_CALL(*mocked_store_, Load(_)).WillOnce(testing::SaveArg<0>(&callback));
  tracker_->Load(load_callback_);

  std::map<const base::Feature*, uint32_t> result_map;
  result_map[&kTestFeatureFoo] = 100u;  // In filter.
  result_map[&kTestFeatureBar] = 200u;  // In filter.
  result_map[&kTestFeatureQux] = 300u;  // Not in filter.

  callback.Run(true, base::MakeUnique<std::map<const base::Feature*, uint32_t>>(
                         result_map));
  ASSERT_TRUE(load_success_.value());

  ASSERT_TRUE(tracker_->HasAvailabilityForTest(kTestFeatureFoo));
  ASSERT_TRUE(tracker_->HasAvailabilityForTest(kTestFeatureBar));
  EXPECT_FALSE(tracker_->HasAvailabilityForTest(kTestFeatureQux));
  ASSERT_TRUE(tracker_->HasAvailabilityForTest(kTestFeatureNop));

  // The foo and bar features had values in the DB.
  EXPECT_EQ(100u, tracker_->GetAvailability(kTestFeatureFoo));
  EXPECT_EQ(200u, tracker_->GetAvailability(kTestFeatureBar));

  // The nop-feature should have been inserted with 0u as the value.
  EXPECT_EQ(0u, tracker_->GetAvailability(kTestFeatureNop));

  // Verify that everything has now been cleaned up.
  EXPECT_TRUE(store_destructed_);
  EXPECT_FALSE(tracker_->HasFilterForTest());
}

TEST_F(AvailabilityTrackerImplTest, FailLoad) {
  FeatureVector features;
  features.push_back(&kTestFeatureFoo);
  SetUpTracker(features);

  AvailabilityStore::OnLoadedCallback callback;
  EXPECT_CALL(*mocked_store_, Load(_)).WillOnce(testing::SaveArg<0>(&callback));
  tracker_->Load(load_callback_);

  callback.Run(false,
               base::MakeUnique<std::map<const base::Feature*, uint32_t>>());
  EXPECT_FALSE(load_success_.value());

  EXPECT_FALSE(tracker_->HasAvailabilityForTest(kTestFeatureFoo));
  EXPECT_FALSE(tracker_->HasAvailabilityForTest(kTestFeatureBar));
  EXPECT_FALSE(tracker_->HasAvailabilityForTest(kTestFeatureQux));
  EXPECT_FALSE(tracker_->HasAvailabilityForTest(kTestFeatureNop));

  // Verify that everything has now been cleaned up.
  EXPECT_TRUE(store_destructed_);
  EXPECT_FALSE(tracker_->HasFilterForTest());
}

}  // namespace feature_engagement_tracker
