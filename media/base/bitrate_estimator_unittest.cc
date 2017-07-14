// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/bitrate_estimator.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/simple_test_tick_clock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

namespace {
constexpr int kFrameSize = 100;
constexpr int kBitsPerByte = 8;
}  // namespace

class BitrateEstimatorTest : public ::testing::Test {
 public:
  BitrateEstimatorTest() {}
  ~BitrateEstimatorTest() override {}

  void TearDown() final { RunUntilIdle(); }

  static void RunUntilIdle() { base::RunLoop().RunUntilIdle(); }

  void Start() {
    EXPECT_FALSE(estimator_);
    EXPECT_FALSE(testing_clock_);
    estimator_ = base::MakeUnique<BitrateEstimator>();
    testing_clock_ = new base::SimpleTestTickClock();
    estimator_->clock_.reset(testing_clock_);
    estimator_->Reset();
  }

  base::MessageLoop message_loop_;

 protected:
  std::unique_ptr<BitrateEstimator> estimator_;
  base::SimpleTestTickClock* testing_clock_ = nullptr;  // Own by |estimator_|.

 private:
  DISALLOW_COPY_AND_ASSIGN(BitrateEstimatorTest);
};

TEST_F(BitrateEstimatorTest, NoFrameEnqueued) {
  Start();
  RunUntilIdle();
  EXPECT_EQ(0.0, estimator_->GetBitrate());
}

TEST_F(BitrateEstimatorTest, NormalEstimation) {
  Start();
  RunUntilIdle();
  estimator_->EnqueueOneFrame(kFrameSize);
  testing_clock_->Advance(base::TimeDelta::FromSeconds(1));
  RunUntilIdle();
  EXPECT_EQ(kFrameSize * kBitsPerByte, estimator_->GetBitrate());
  estimator_->EnqueueOneFrame(kFrameSize);
  testing_clock_->Advance(base::TimeDelta::FromSeconds(1));
  RunUntilIdle();
  EXPECT_EQ(kFrameSize * kBitsPerByte, estimator_->GetBitrate());
}

TEST_F(BitrateEstimatorTest, ResetDuringEstimation) {
  Start();
  RunUntilIdle();
  estimator_->EnqueueOneFrame(kFrameSize);
  testing_clock_->Advance(base::TimeDelta::FromSeconds(1));
  RunUntilIdle();
  EXPECT_EQ(kFrameSize * kBitsPerByte, estimator_->GetBitrate());
  estimator_->Reset();
  testing_clock_->Advance(base::TimeDelta::FromSeconds(1));
  RunUntilIdle();
  EXPECT_EQ(0, estimator_->GetBitrate());
}

}  // namespace media
