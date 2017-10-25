// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/ukm_manager.h"

#include "components/ukm/test_ukm_recorder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {
const char kUserInteraction[] = "Compositor.UserInteraction";
const char kCheckerboardArea[] = "CheckerboardedContentArea";
const char kMissingTiles[] = "NumMissingTiles";

class UkmRecorderForTest : public ukm::TestUkmRecorder {
 public:
  UkmRecorderForTest() = default;
  ~UkmRecorderForTest() override {
    if (has_no_entries_) {
      EXPECT_FALSE(HasEntry(*source_, kUserInteraction));
      return;
    }

    ExpectMetrics(*source_, kUserInteraction, kCheckerboardArea,
                  final_checkerboard_);
    ExpectMetrics(*source_, kUserInteraction, kMissingTiles,
                  final_missing_tiles_);
  }

  bool has_no_entries_ = true;
  std::vector<int64_t> final_checkerboard_;
  std::vector<int64_t> final_missing_tiles_;
  ukm::UkmSource* source_ = nullptr;
};

class UkmManagerTest : public testing::Test {
 public:
  void SetUp() override { UpdateUkmRecorder(GURL("test")); }

  void TearDown() override {
    recorder_ = nullptr;
    ukm_source_ = nullptr;
  }

 protected:
  void UpdateUkmRecorder(const GURL& url) {
    auto owned_recorder = std::make_unique<UkmRecorderForTest>();
    recorder_ = owned_recorder.get();

    auto source_id = recorder_->GetNewSourceID();
    static_cast<ukm::UkmRecorder*>(recorder_)->UpdateSourceURL(source_id, url);
    ASSERT_EQ(recorder_->GetSourceIds().count(source_id), 1u);

    ukm_source_ =
        const_cast<ukm::UkmSource*>(recorder_->GetSourceForSourceId(source_id));
    ASSERT_TRUE(ukm_source_);
    recorder_->source_ = ukm_source_;

    manager_.UpdateRecorder(std::move(owned_recorder), source_id);
  }

  UkmManager manager_;
  UkmRecorderForTest* recorder_;
  ukm::UkmSource* ukm_source_;
};

TEST_F(UkmManagerTest, Basic) {
  manager_.SetUserInteractionInProgress(true);
  manager_.AddCheckerboardStatsForFrame(5, 1);
  manager_.AddCheckerboardStatsForFrame(15, 3);
  manager_.SetUserInteractionInProgress(false);

  // We should see a single entry for the interaction above.
  recorder_->has_no_entries_ = false;
  EXPECT_EQ(recorder_->entries_count(), 1u);
  EXPECT_TRUE(recorder_->HasEntry(*ukm_source_, kUserInteraction));
  recorder_->ExpectMetric(*ukm_source_, kUserInteraction, kCheckerboardArea,
                          10);
  recorder_->ExpectMetric(*ukm_source_, kUserInteraction, kMissingTiles, 2);

  // Try pushing some stats while no user interaction is happening. No entries
  // should be pushed.
  manager_.AddCheckerboardStatsForFrame(6, 1);
  manager_.AddCheckerboardStatsForFrame(99, 3);
  EXPECT_EQ(recorder_->entries_count(), 1u);
  manager_.SetUserInteractionInProgress(true);
  EXPECT_EQ(recorder_->entries_count(), 1u);

  // Record a few entries and change the source before the interaction ends. The
  // stats collected up till this point should be recorded before the source is
  // swapped.
  manager_.AddCheckerboardStatsForFrame(10, 1);
  manager_.AddCheckerboardStatsForFrame(30, 5);
  recorder_->final_checkerboard_ = {10, 20};
  recorder_->final_missing_tiles_ = {2, 3};

  UpdateUkmRecorder(GURL("test2"));
}

}  // namespace
}  // namespace cc
