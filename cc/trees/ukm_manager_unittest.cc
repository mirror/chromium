// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/ukm_manager.h"

#include "components/ukm/test_ukm_recorder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {
const char kUserInteraction[] = "Compositor.UserInteraction";
const char kRendering[] = "Compositor.Rendering";
const char kCheckerboardArea[] = "CheckerboardedContentArea";
const char kCheckerboardAreaRatio[] = "CheckerboardedContentAreaRatio";
const char kMissingTiles[] = "NumMissingTiles";
const char kCheckerboardedImagesCount[] = "CheckerboardedImagesCount";

class UkmRecorderForTest : public ukm::TestUkmRecorder {
 public:
  UkmRecorderForTest() = default;
  ~UkmRecorderForTest() override {
    ExpectMetrics(*source_, kUserInteraction, kCheckerboardArea,
                  expected_final_checkerboard_);
    ExpectMetrics(*source_, kUserInteraction, kMissingTiles,
                  expected_final_missing_tiles_);
    ExpectMetrics(*source_, kUserInteraction, kCheckerboardAreaRatio,
                  expected_final_checkerboard_ratio_);
    ExpectMetrics(*source_, kUserInteraction, kCheckerboardedImagesCount,
                  expected_final_checkerboarded_images_);

    ExpectMetrics(*source_, kRendering, kCheckerboardedImagesCount,
                  expected_final_total_checkerboarded_images_);
  }

  std::vector<int64_t> expected_final_checkerboard_;
  std::vector<int64_t> expected_final_missing_tiles_;
  std::vector<int64_t> expected_final_checkerboard_ratio_;
  std::vector<int64_t> expected_final_checkerboarded_images_;

  std::vector<int64_t> expected_final_total_checkerboarded_images_;

  ukm::UkmSource* source_ = nullptr;
};

class UkmManagerTest : public testing::Test {
 public:
  UkmManagerTest() : manager_(std::make_unique<UkmRecorderForTest>()) {
    UpdateURL(GURL("chrome://test"));
  }

  void TearDown() override { ukm_source_ = nullptr; }

  UkmRecorderForTest* recorder() {
    return static_cast<UkmRecorderForTest*>(manager_.recorder_for_testing());
  }

 protected:
  void UpdateURL(const GURL& url) {
    manager_.SetSourceURL(url);
    ukm_source_ = const_cast<ukm::UkmSource*>(recorder()->GetSourceForUrl(url));
  }

  UkmManager manager_;
  ukm::UkmSource* ukm_source_ = nullptr;
};

TEST_F(UkmManagerTest, Basic) {
  manager_.SetUserInteractionInProgress(true);
  manager_.AddCheckerboardStatsForFrame(5, 1, 10);
  manager_.AddCheckerboardStatsForFrame(15, 3, 30);
  manager_.AddCheckerboardedImages(6);
  manager_.SetUserInteractionInProgress(false);

  // We should see a single entry for the interaction above.
  EXPECT_EQ(recorder()->entries_count(), 1u);
  EXPECT_TRUE(recorder()->HasEntry(*ukm_source_, kUserInteraction));
  recorder()->ExpectMetric(*ukm_source_, kUserInteraction, kCheckerboardArea,
                           10);
  recorder()->ExpectMetric(*ukm_source_, kUserInteraction, kMissingTiles, 2);
  recorder()->ExpectMetric(*ukm_source_, kUserInteraction,
                           kCheckerboardAreaRatio, 50);
  recorder()->ExpectMetric(*ukm_source_, kUserInteraction,
                           kCheckerboardedImagesCount, 6);

  // Try pushing some stats while no user interaction is happening. No entries
  // should be pushed.
  manager_.AddCheckerboardStatsForFrame(6, 1, 10);
  manager_.AddCheckerboardStatsForFrame(99, 3, 100);
  EXPECT_EQ(recorder()->entries_count(), 1u);
  manager_.SetUserInteractionInProgress(true);
  EXPECT_EQ(recorder()->entries_count(), 1u);

  // Record a few entries and change the source before the interaction ends. The
  // stats collected up till this point should be recorded before the source is
  // swapped.
  manager_.AddCheckerboardStatsForFrame(10, 1, 100);
  manager_.AddCheckerboardStatsForFrame(30, 5, 100);
  recorder()->expected_final_checkerboard_ = {10, 20};
  recorder()->expected_final_missing_tiles_ = {2, 3};
  recorder()->expected_final_checkerboard_ratio_ = {50, 20};
  recorder()->expected_final_checkerboarded_images_ = {6, 0};
  recorder()->expected_final_total_checkerboarded_images_ = {6};
  recorder()->source_ = ukm_source_;

  UpdateURL(GURL("chrome://test2"));
}

}  // namespace
}  // namespace cc
