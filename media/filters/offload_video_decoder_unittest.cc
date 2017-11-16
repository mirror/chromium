// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/filters/offload_video_decoder.h"

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "media/base/decoder_buffer.h"
#include "media/base/gmock_callback_support.h"
#include "media/base/mock_filters.h"
#include "media/base/test_data_util.h"
#include "media/base/test_helpers.h"
#include "media/base/video_frame.h"
#include "testing/gmock/include/gmock/gmock.h"

using testing::_;
using testing::DoAll;
using testing::SaveArg;

namespace media {

ACTION_P(VerifyOn, task_runner) {
  ASSERT_TRUE(task_runner->RunsTasksInCurrentSequence());
}

ACTION_P(VerifyNotOn, task_runner) {
  ASSERT_FALSE(task_runner->RunsTasksInCurrentSequence());
}

class OffloadVideoDecoderTest : public testing::Test {
 public:
  OffloadVideoDecoderTest()
      : task_env_(base::test::ScopedTaskEnvironment::MainThreadType::DEFAULT,
                  base::test::ScopedTaskEnvironment::ExecutionMode::QUEUED) {}

  void CreateWrapper(int offload_width, VideoCodec codec) {
    decoder_ = new testing::StrictMock<MockVideoDecoder>();
    offload_wrapper_ = std::make_unique<OffloadVideoDecoder>(
        offload_width, std::vector<VideoCodec>(1, codec),
        std::unique_ptr<VideoDecoder>(decoder_));
  }

  VideoDecoder::InitCB ExpectInitCB(bool success) {
    EXPECT_CALL(*this, InitDone(success))
        .WillOnce(VerifyOn(task_env_.GetMainThreadTaskRunner()));
    return base::Bind(&OffloadVideoDecoderTest::InitDone,
                      base::Unretained(this));
  }

  VideoDecoder::OutputCB ExpectOutputCB() {
    EXPECT_CALL(*this, OutputDone(_))
        .WillOnce(VerifyOn(task_env_.GetMainThreadTaskRunner()));
    return base::Bind(&OffloadVideoDecoderTest::OutputDone,
                      base::Unretained(this));
  }

  VideoDecoder::DecodeCB ExpectDecodeCB(DecodeStatus status) {
    EXPECT_CALL(*this, DecodeDone(status))
        .WillOnce(VerifyOn(task_env_.GetMainThreadTaskRunner()));
    return base::Bind(&OffloadVideoDecoderTest::DecodeDone,
                      base::Unretained(this));
  }

  base::Closure ExpectResetCB() {
    EXPECT_CALL(*this, ResetDone())
        .WillOnce(VerifyOn(task_env_.GetMainThreadTaskRunner()));
    return base::Bind(&OffloadVideoDecoderTest::ResetDone,
                      base::Unretained(this));
  }

  MOCK_METHOD1(InitDone, void(bool));
  MOCK_METHOD1(OutputDone, void(const scoped_refptr<VideoFrame>&));
  MOCK_METHOD1(DecodeDone, void(DecodeStatus));
  MOCK_METHOD0(ResetDone, void(void));

  base::test::ScopedTaskEnvironment task_env_;
  std::unique_ptr<OffloadVideoDecoder> offload_wrapper_;
  testing::StrictMock<MockVideoDecoder>* decoder_ =
      nullptr;  // Owned by |offload_wrapper_|.

 private:
  DISALLOW_COPY_AND_ASSIGN(OffloadVideoDecoderTest);
};

TEST_F(OffloadVideoDecoderTest, BasicFunctionality) {
  auto offload_config = TestVideoConfig::Large(kCodecVP9);
  CreateWrapper(offload_config.coded_size().width(), kCodecVP9);

  // Display name should be a simple passthrough.
  EXPECT_EQ(offload_wrapper_->GetDisplayName(), decoder_->GetDisplayName());

  // Verify methods are called on the current thread since the offload codec
  // requirement is not satisfied.
  VideoDecoder::OutputCB output_cb;
  EXPECT_CALL(*decoder_, Initialize(_, false, nullptr, _, _))
      .WillOnce(DoAll(VerifyOn(task_env_.GetMainThreadTaskRunner()),
                      RunCallback<3>(true), SaveArg<4>(&output_cb)));
  offload_wrapper_->Initialize(TestVideoConfig::Normal(kCodecVP9), false,
                               nullptr, ExpectInitCB(true), ExpectOutputCB());
  task_env_.RunUntilIdle();

  // Verify decode works and is called on the right thread.
  EXPECT_CALL(*decoder_, Decode(_, _))
      .WillOnce(DoAll(VerifyOn(task_env_.GetMainThreadTaskRunner()),
                      RunClosure(base::Bind(output_cb, nullptr)),
                      RunCallback<1>(DecodeStatus::OK)));
  offload_wrapper_->Decode(DecoderBuffer::CreateEOSBuffer(),
                           ExpectDecodeCB(DecodeStatus::OK));
  task_env_.RunUntilIdle();

  // Reset so we can call Initialize() again.
  EXPECT_CALL(*decoder_, Reset(_))
      .WillOnce(DoAll(VerifyOn(task_env_.GetMainThreadTaskRunner()),
                      RunCallback<0>()));
  offload_wrapper_->Reset(ExpectResetCB());
  task_env_.RunUntilIdle();

  // Reinitialize decoder with a stream which should be offloaded. Since this
  // Initialize() should be happening on another thread, set the expectation
  // after we make the call.
  offload_wrapper_->Initialize(offload_config, false, nullptr,
                               ExpectInitCB(true), ExpectOutputCB());
  EXPECT_CALL(*decoder_, Initialize(_, false, nullptr, _, _))
      .WillOnce(DoAll(VerifyNotOn(task_env_.GetMainThreadTaskRunner()),
                      RunCallback<3>(true), SaveArg<4>(&output_cb)));
  task_env_.RunUntilIdle();

  // Verify decode works and is called on the right thread.
  offload_wrapper_->Decode(DecoderBuffer::CreateEOSBuffer(),
                           ExpectDecodeCB(DecodeStatus::OK));
  EXPECT_CALL(*decoder_, Decode(_, _))
      .WillOnce(DoAll(VerifyNotOn(task_env_.GetMainThreadTaskRunner()),
                      RunClosure(base::Bind(output_cb, nullptr)),
                      RunCallback<1>(DecodeStatus::OK)));
  task_env_.RunUntilIdle();

  // Reset so we can call Initialize() again.
  offload_wrapper_->Reset(ExpectResetCB());
  EXPECT_CALL(*decoder_, Reset(_))
      .WillOnce(DoAll(VerifyNotOn(task_env_.GetMainThreadTaskRunner()),
                      RunCallback<0>()));
  task_env_.RunUntilIdle();
}

}  // namespace media
