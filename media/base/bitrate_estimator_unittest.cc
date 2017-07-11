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
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

namespace {

constexpr int kFrameSize = 100;
constexpr int kLargeFrameSize = 200;
constexpr int kBitsPerByte = 8;
constexpr base::TimeDelta kEstimateDuration = base::TimeDelta::FromSeconds(3);
constexpr base::TimeDelta kFirstFrameStamp = base::TimeDelta::FromSeconds(1);
constexpr base::TimeDelta kInterval = base::TimeDelta::FromSeconds(1);

int GetBitrateForFrames(int num_of_frames, int frame_size) {
  return num_of_frames * frame_size * kBitsPerByte /
         (kInterval.InSeconds() * (num_of_frames - 1));
}

}  // namespace

class BitrateEstimatorTest : public ::testing::Test {
 public:
  BitrateEstimatorTest() {}
  ~BitrateEstimatorTest() override {}

  void TearDown() final { RunUntilIdle(); }

  static void RunUntilIdle() { base::RunLoop().RunUntilIdle(); }

  void OnBitrateEstimated(BitrateEstimator::Status status, int bitrate) {
    status_ = status;
    bitrate_ = bitrate;
    ++num_cb_called_;
  }

  base::MessageLoop message_loop_;

 protected:
  std::unique_ptr<BitrateEstimator> estimator_;
  BitrateEstimator::Status status_ = BitrateEstimator::Status::kNone;
  int bitrate_ = 0;
  int num_cb_called_ = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(BitrateEstimatorTest);
};

TEST_F(BitrateEstimatorTest, NoFrameEnqueued) {
  estimator_ = base::MakeUnique<BitrateEstimator>(
      kEstimateDuration, base::Bind(&BitrateEstimatorTest::OnBitrateEstimated,
                                    base::Unretained(this)));
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 0);
  EXPECT_EQ(status_, BitrateEstimator::Status::kNone);
  estimator_->ReportResult();
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 1);
  EXPECT_EQ(status_, BitrateEstimator::Status::kAborted);
  EXPECT_EQ(bitrate_, 0);
}

TEST_F(BitrateEstimatorTest, OneFrameEnqueued) {
  estimator_ = base::MakeUnique<BitrateEstimator>(
      kEstimateDuration, base::Bind(&BitrateEstimatorTest::OnBitrateEstimated,
                                    base::Unretained(this)));
  RunUntilIdle();
  estimator_->EnqueueOneFrame(kFrameSize, kFirstFrameStamp, true);
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 0);
  EXPECT_EQ(status_, BitrateEstimator::Status::kNone);
  estimator_->ReportResult();
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 1);
  EXPECT_EQ(status_, BitrateEstimator::Status::kAborted);
  EXPECT_EQ(bitrate_, 0);
}

TEST_F(BitrateEstimatorTest, AbortEarly) {
  estimator_ = base::MakeUnique<BitrateEstimator>(
      kEstimateDuration, base::Bind(&BitrateEstimatorTest::OnBitrateEstimated,
                                    base::Unretained(this)));
  RunUntilIdle();
  estimator_->EnqueueOneFrame(kFrameSize, kFirstFrameStamp, true);
  RunUntilIdle();
  estimator_->EnqueueOneFrame(kFrameSize, kFirstFrameStamp + kInterval, false);
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 0);
  EXPECT_EQ(status_, BitrateEstimator::Status::kNone);
  estimator_->ReportResult();
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 1);
  EXPECT_EQ(status_, BitrateEstimator::Status::kAborted);
  EXPECT_EQ(bitrate_, GetBitrateForFrames(2, kFrameSize));
}

TEST_F(BitrateEstimatorTest, NormalEstimation) {
  estimator_ = base::MakeUnique<BitrateEstimator>(
      kEstimateDuration, base::Bind(&BitrateEstimatorTest::OnBitrateEstimated,
                                    base::Unretained(this)));
  RunUntilIdle();
  base::TimeDelta timestamp = kFirstFrameStamp;
  // Enqueue 4 frames.
  for (int i = 0; i < 4; ++i) {
    estimator_->EnqueueOneFrame(kFrameSize, timestamp, i ? false : true);
    RunUntilIdle();
    EXPECT_EQ(num_cb_called_, 0);
    timestamp += kInterval;
  }
  // Enqueue the 5th frame.
  estimator_->EnqueueOneFrame(kFrameSize, timestamp, false);
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 1);
  EXPECT_EQ(status_, BitrateEstimator::Status::kOk);
  // The reported bitrate should not include the 5th frame.
  EXPECT_EQ(bitrate_, GetBitrateForFrames(4, kFrameSize));
}

TEST_F(BitrateEstimatorTest, NotStartFromKeyFrame) {
  estimator_ = base::MakeUnique<BitrateEstimator>(
      kEstimateDuration, base::Bind(&BitrateEstimatorTest::OnBitrateEstimated,
                                    base::Unretained(this)));
  RunUntilIdle();
  base::TimeDelta timestamp = kFirstFrameStamp;
  // Enqueue 4 frames.
  for (int i = 0; i < 4; ++i) {
    estimator_->EnqueueOneFrame(kFrameSize, timestamp, false);
    RunUntilIdle();
    EXPECT_EQ(num_cb_called_, 0);
    timestamp += kInterval;
  }
  // Enqueue other 4 frames.
  for (int i = 0; i < 4; ++i) {
    estimator_->EnqueueOneFrame(kLargeFrameSize, timestamp, i ? false : true);
    RunUntilIdle();
    EXPECT_EQ(num_cb_called_, 0);
    timestamp += kInterval;
  }
  // Enqueue the 5th frame.
  estimator_->EnqueueOneFrame(kFrameSize, timestamp, false);
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 1);
  EXPECT_EQ(status_, BitrateEstimator::Status::kOk);
  // The reported bitrate should not include the 5th frame.
  EXPECT_EQ(bitrate_, GetBitrateForFrames(4, kLargeFrameSize));
}

TEST_F(BitrateEstimatorTest, RepeatEstimation) {
  estimator_ = base::MakeUnique<BitrateEstimator>(
      kEstimateDuration, base::Bind(&BitrateEstimatorTest::OnBitrateEstimated,
                                    base::Unretained(this)));
  RunUntilIdle();
  base::TimeDelta timestamp = kFirstFrameStamp;
  // Enqueue 4 frames.
  for (int i = 0; i < 4; ++i) {
    estimator_->EnqueueOneFrame(kFrameSize, timestamp, i ? false : true);
    RunUntilIdle();
    EXPECT_EQ(num_cb_called_, 0);
    timestamp += kInterval;
  }
  // Enqueue the 5th frame.
  estimator_->EnqueueOneFrame(kLargeFrameSize, timestamp, false);
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 1);
  EXPECT_EQ(status_, BitrateEstimator::Status::kOk);
  EXPECT_EQ(bitrate_, GetBitrateForFrames(4, kFrameSize));
  // Enqueue other 3 frames.
  timestamp += kInterval;
  for (int i = 0; i < 3; ++i) {
    estimator_->EnqueueOneFrame(kLargeFrameSize, timestamp, false);
    RunUntilIdle();
    EXPECT_EQ(num_cb_called_, 1);
    timestamp += kInterval;
  }
  // End of stream.
  estimator_->ReportResult();
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 2);
  EXPECT_EQ(status_, BitrateEstimator::Status::kOk);
  EXPECT_EQ(bitrate_, GetBitrateForFrames(4, kLargeFrameSize));
}

TEST_F(BitrateEstimatorTest, EnqueuedInDecodingOrder) {
  estimator_ = base::MakeUnique<BitrateEstimator>(
      kEstimateDuration, base::Bind(&BitrateEstimatorTest::OnBitrateEstimated,
                                    base::Unretained(this)));
  RunUntilIdle();
  // Enqueue 4 frames in decoding order. The GOP structure is I B1 B2 P.
  // I
  estimator_->EnqueueOneFrame(kFrameSize, kFirstFrameStamp, true);
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 0);
  // P
  estimator_->EnqueueOneFrame(kFrameSize, kFirstFrameStamp + kInterval * 3,
                              false);
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 0);
  // B1
  estimator_->EnqueueOneFrame(kFrameSize, kFirstFrameStamp + kInterval, false);
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 0);
  // B2
  estimator_->EnqueueOneFrame(kFrameSize, kFirstFrameStamp + kInterval * 2,
                              false);
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 0);

  // Enqueue the 5th frame.
  estimator_->EnqueueOneFrame(kFrameSize, kFirstFrameStamp + kInterval * 4,
                              false);
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 1);
  EXPECT_EQ(status_, BitrateEstimator::Status::kOk);
  EXPECT_EQ(bitrate_, GetBitrateForFrames(4, kFrameSize));
}

TEST_F(BitrateEstimatorTest, InaccurateEstimation) {
  estimator_ = base::MakeUnique<BitrateEstimator>(
      kEstimateDuration, base::Bind(&BitrateEstimatorTest::OnBitrateEstimated,
                                    base::Unretained(this)));
  RunUntilIdle();
  // Enqueue 4 frames in decoding order. The GOP structure is I B1 B2 P1 P2.
  // Decoding order: I P1 B1 P2 B2
  // I
  estimator_->EnqueueOneFrame(kFrameSize, kFirstFrameStamp, true);
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 0);
  // P1
  estimator_->EnqueueOneFrame(kFrameSize, kFirstFrameStamp + kInterval * 3,
                              false);
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 0);
  // B1
  estimator_->EnqueueOneFrame(kFrameSize, kFirstFrameStamp + kInterval, false);
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 0);
  // P2
  estimator_->EnqueueOneFrame(kFrameSize, kFirstFrameStamp + kInterval * 4,
                              false);
  RunUntilIdle();
  EXPECT_EQ(num_cb_called_, 1);
  EXPECT_EQ(status_, BitrateEstimator::Status::kOk);
  EXPECT_LT(bitrate_, GetBitrateForFrames(4, kFrameSize));
}

}  // namespace media
