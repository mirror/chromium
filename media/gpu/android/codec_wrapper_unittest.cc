// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/codec_wrapper.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "media/base/android/media_codec_bridge.h"
#include "media/base/android/mock_media_codec_bridge.h"
#include "media/base/encryption_scheme.h"
#include "media/base/subsample_entry.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Invoke;
using testing::Return;
using testing::DoAll;
using testing::SetArgPointee;
using testing::NiceMock;
using testing::_;

namespace media {

class CodecWrapperTest : public testing::Test {
 public:
  CodecWrapperTest() {
    auto codec = base::MakeUnique<NiceMock<MockMediaCodecBridge>>();
    codec_ = codec.get();
    wrapper_ = new CodecWrapper(std::move(codec));
    ON_CALL(*codec_, DequeueOutputBuffer(_, _, _, _, _, _, _))
        .WillByDefault(Return(MEDIA_CODEC_OK));
  }

  ~CodecWrapperTest() override {
    // ~CodecWrapper asserts that the codec was taken.
    wrapper_->TakeCodec();
  }

  std::unique_ptr<CodecBuffer> DequeueCodecBuffer() {
    std::unique_ptr<CodecBuffer> codec_buffer;
    wrapper_->DequeueOutputBuffer(base::TimeDelta(), nullptr, nullptr,
                                  &codec_buffer);
    return codec_buffer;
  }

  NiceMock<MockMediaCodecBridge>* codec_;
  scoped_refptr<CodecWrapper> wrapper_;
};

TEST_F(CodecWrapperTest, CanTakeCodec) {
  ASSERT_EQ(wrapper_->TakeCodec().get(), codec_);
}

TEST_F(CodecWrapperTest, NoCodecBufferReturnedIfDequeueFails) {
  EXPECT_CALL(*codec_, DequeueOutputBuffer(_, _, _, _, _, _, _))
      .WillOnce(Return(MEDIA_CODEC_ERROR));
  auto codec_buffer = DequeueCodecBuffer();
  ASSERT_EQ(codec_buffer, nullptr);
}

TEST_F(CodecWrapperTest, InitiallyThereAreNoValidCodecBuffers) {
  ASSERT_FALSE(wrapper_->HasValidCodecBuffers());
}

TEST_F(CodecWrapperTest, CodecBuffersAreInitiallyValid) {
  auto codec_buffer = DequeueCodecBuffer();
  ASSERT_TRUE(codec_buffer->IsValid());
  ASSERT_TRUE(wrapper_->HasValidCodecBuffers());
}

TEST_F(CodecWrapperTest, FlushInvalidatesCodecBuffers) {
  auto codec_buffer = DequeueCodecBuffer();
  wrapper_->Flush();
  ASSERT_FALSE(codec_buffer->IsValid());
}

TEST_F(CodecWrapperTest, TakingTheCodecInvalidatesCodecBuffers) {
  auto codec_buffer = DequeueCodecBuffer();
  wrapper_->TakeCodec();
  ASSERT_FALSE(codec_buffer->IsValid());
}

TEST_F(CodecWrapperTest, SetSurfaceInvalidatesCodecBuffers) {
  auto codec_buffer = DequeueCodecBuffer();
  wrapper_->SetSurface(0);
  ASSERT_FALSE(codec_buffer->IsValid());
}

TEST_F(CodecWrapperTest, CodecBuffersAreAllInvalidatedTogether) {
  auto codec_buffer1 = DequeueCodecBuffer();
  auto codec_buffer2 = DequeueCodecBuffer();
  wrapper_->Flush();
  ASSERT_FALSE(codec_buffer1->IsValid());
  ASSERT_FALSE(codec_buffer2->IsValid());
  ASSERT_FALSE(wrapper_->HasValidCodecBuffers());
}

TEST_F(CodecWrapperTest, CodecBuffersAfterFlushAreValid) {
  auto codec_buffer = DequeueCodecBuffer();
  wrapper_->Flush();
  codec_buffer = DequeueCodecBuffer();
  ASSERT_TRUE(codec_buffer->IsValid());
}

TEST_F(CodecWrapperTest, CodecBufferReleaseUsesCorrectIndex) {
  // The second arg is the buffer index pointer.
  EXPECT_CALL(*codec_, DequeueOutputBuffer(_, _, _, _, _, _, _))
      .WillOnce(DoAll(SetArgPointee<1>(42), Return(MEDIA_CODEC_OK)));
  auto codec_buffer = DequeueCodecBuffer();
  EXPECT_CALL(*codec_, ReleaseOutputBuffer(42, true));
  codec_buffer->ReleaseToSurface();
}

TEST_F(CodecWrapperTest, CodecBuffersAreInvalidatedByRelease) {
  auto codec_buffer = DequeueCodecBuffer();
  codec_buffer->ReleaseToSurface();
  ASSERT_FALSE(codec_buffer->IsValid());
}

TEST_F(CodecWrapperTest, CodecBuffersReleaseOnDestruction) {
  auto codec_buffer = DequeueCodecBuffer();
  EXPECT_CALL(*codec_, ReleaseOutputBuffer(_, false));
  codec_buffer = nullptr;
}

TEST_F(CodecWrapperTest, CodecBuffersDoNotReleaseIfAlreadyReleased) {
  auto codec_buffer = DequeueCodecBuffer();
  codec_buffer->ReleaseToSurface();
  EXPECT_CALL(*codec_, ReleaseOutputBuffer(_, _)).Times(0);
  codec_buffer = nullptr;
}

TEST_F(CodecWrapperTest, ReleasingCodecBuffersAfterTheCodecIsSafe) {
  auto codec_buffer = DequeueCodecBuffer();
  wrapper_->TakeCodec();
  codec_buffer->ReleaseToSurface();
}

TEST_F(CodecWrapperTest, DeletingCodecBuffersAfterTheCodecIsSafe) {
  auto codec_buffer = DequeueCodecBuffer();
  wrapper_->TakeCodec();
  // This test ensures the destructor doesn't crash.
  codec_buffer = nullptr;
}

TEST_F(CodecWrapperTest, CodecBufferReleaseDoesNotInvalidateOthers) {
  auto codec_buffer1 = DequeueCodecBuffer();
  auto codec_buffer2 = DequeueCodecBuffer();
  codec_buffer1->ReleaseToSurface();
  ASSERT_TRUE(codec_buffer2->IsValid());
}

#if !defined(NDEBUG)
TEST_F(CodecWrapperTest, CallsDcheckAfterTakingTheCodec) {
  wrapper_->TakeCodec();
  ASSERT_DEATH(wrapper_->Flush(), "");
}

TEST_F(CodecWrapperTest, CallsDcheckAfterAnError) {
  EXPECT_CALL(*codec_, Flush()).WillOnce(Return(MEDIA_CODEC_ERROR));
  wrapper_->Flush();
  ASSERT_DEATH(wrapper_->SetSurface(0), "");
}
#endif

}  // namespace media
