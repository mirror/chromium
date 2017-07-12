// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/message_loop/message_loop.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "media/mojo/interfaces/video_encode_accelerator.mojom.h"
#include "media/mojo/services/mojo_video_encode_accelerator.h"
#include "media/video/fake_video_encode_accelerator.h"
#include "media/video/video_encode_accelerator.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gpu {
struct GpuPreferences;
}  // namespace gpu

using ::testing::_;

namespace media {

static const gfx::Size kInputVisibleSize(64, 48);

std::unique_ptr<VideoEncodeAccelerator> CreateAndInitializeFakeVEA(
    bool will_initialization_succeed,
    VideoPixelFormat input_format,
    const gfx::Size& input_visible_size,
    VideoCodecProfile output_profile,
    uint32_t initial_bitrate,
    VideoEncodeAccelerator::Client* client,
    const gpu::GpuPreferences& gpu_preferences) {
  FakeVideoEncodeAccelerator* vea =
      new FakeVideoEncodeAccelerator(base::ThreadTaskRunnerHandle::Get());
  vea->SetWillInitializationSucceed(will_initialization_succeed);
  const bool result = vea->Initialize(input_format, input_visible_size,
                                      output_profile, initial_bitrate, client);

  // Mimic the behaviour of GpuVideoEncodeAcceleratorFactory::CreateVEA().
  return result ? base::WrapUnique<VideoEncodeAccelerator>(vea) : nullptr;
}

class MockMojoVideoEncodeAcceleratorClient
    : public mojom::VideoEncodeAcceleratorClient {
 public:
  MockMojoVideoEncodeAcceleratorClient() = default;

  MOCK_METHOD3(RequireBitstreamBuffers,
               void(uint32_t, const gfx::Size&, uint32_t));
  MOCK_METHOD4(BitstreamBufferReady,
               void(int32_t, uint32_t, bool, base::TimeDelta));
  MOCK_METHOD1(NotifyError, void(mojom::VideoEncodeAccelerator::Error));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockMojoVideoEncodeAcceleratorClient);
};

// Test harness for a MojoVideoEncodeAccelerator; the tests manipulate it via
// its MojoVideoEncodeAccelerator interface while observing a "remote"
// mojo::VideoEncodeAcceleratorClient (that we keep inside a Mojo binding). The
// class under test uses a FakeVideoEncodeAccelerator as implementation.
class MojoVideoEncodeAcceleratorTest : public ::testing::Test {
 public:
  MojoVideoEncodeAcceleratorTest() = default;

  void TearDown() override {
    // The destruction of a mojo::StrongBinding closes the bound message pipe
    // but does not destroy the implementation object: needs to happen manually,
    // otherwise we leak it. This only applies if BindAndInitialize() has been
    // called.
    if (mojo_vea_binding_)
      mojo_vea_binding_->Close();
  }

  // Creates the class under test, configuring the underlying FakeVEA to succeed
  // upon initialization (by default) or not.
  void CreateMojoVideoEncodeAccelerator(
      bool will_fake_vea_initialization_succeed = true) {
    mojo_vea_ = base::MakeUnique<MojoVideoEncodeAccelerator>(
        base::Bind(&CreateAndInitializeFakeVEA,
                   will_fake_vea_initialization_succeed),
        gpu::GpuPreferences());
  }

  void BindAndInitialize() {
    // Create an Mojo VEA Client InterfacePtr and point it to bind to our Mock.
    mojom::VideoEncodeAcceleratorClientPtr mojo_vea_client;
    mojo_vea_binding_ = mojo::MakeStrongBinding(
        base::MakeUnique<MockMojoVideoEncodeAcceleratorClient>(),
        mojo::MakeRequest(&mojo_vea_client));

    EXPECT_CALL(*mock_mojo_vea_client(),
                RequireBitstreamBuffers(_, kInputVisibleSize, _));

    const uint32_t kInitialBitrate = 100000u;
    mojo_vea()->Initialize(PIXEL_FORMAT_I420, kInputVisibleSize,
                           H264PROFILE_MIN, kInitialBitrate,
                           std::move(mojo_vea_client));
    base::RunLoop().RunUntilIdle();
  }

  MojoVideoEncodeAccelerator* mojo_vea() { return mojo_vea_.get(); }

  MockMojoVideoEncodeAcceleratorClient* mock_mojo_vea_client() const {
    return static_cast<media::MockMojoVideoEncodeAcceleratorClient*>(
        mojo_vea_binding_->impl());
  }

  FakeVideoEncodeAccelerator* fake_vea() const {
    return static_cast<FakeVideoEncodeAccelerator*>(mojo_vea_->encoder_.get());
  }

 private:
  const base::MessageLoop message_loop_;

  mojo::StrongBindingPtr<mojom::VideoEncodeAcceleratorClient> mojo_vea_binding_;

  // The class under test.
  std::unique_ptr<MojoVideoEncodeAccelerator> mojo_vea_;

  DISALLOW_COPY_AND_ASSIGN(MojoVideoEncodeAcceleratorTest);
};

// This test verifies the BindAndInitialize() communication prologue in
// isolation.
TEST_F(MojoVideoEncodeAcceleratorTest, InitializeAndRequireBistreamBuffers) {
  CreateMojoVideoEncodeAccelerator();
  BindAndInitialize();
}

// This test verifies the BindAndInitialize() communication prologue followed by
// a sharing of a single bitstream buffer and the Encode() of one frame.
TEST_F(MojoVideoEncodeAcceleratorTest, EncodeOneFrame) {
  CreateMojoVideoEncodeAccelerator();
  BindAndInitialize();

  const int32_t kBistreamBufferId = 17;
  {
    const uint64_t kShMemSize = 10;
    auto handle = mojo::SharedBufferHandle::Create(kShMemSize);

    mojo_vea()->UseOutputBitstreamBuffer(kBistreamBufferId, std::move(handle));
    base::RunLoop().RunUntilIdle();
  }

  {
    const auto video_frame = VideoFrame::CreateBlackFrame(kInputVisibleSize);
    EXPECT_CALL(*mock_mojo_vea_client(),
                BitstreamBufferReady(kBistreamBufferId, _, _, _));

    mojo_vea()->Encode(video_frame, true /* is_keyframe */,
                       base::Bind(&base::DoNothing));
    base::RunLoop().RunUntilIdle();
  }
}

// Tests that a RequestEncodingParametersChange() ripples through correctly.
TEST_F(MojoVideoEncodeAcceleratorTest, EncodingParametersChange) {
  CreateMojoVideoEncodeAccelerator();
  BindAndInitialize();

  const uint32_t kNewBitrate = 123123u;
  const uint32_t kNewFramerate = 321321u;
  mojo_vea()->RequestEncodingParametersChange(kNewBitrate, kNewFramerate);
  base::RunLoop().RunUntilIdle();

  ASSERT_TRUE(fake_vea());
  EXPECT_EQ(kNewBitrate, fake_vea()->stored_bitrates().front());
}

// This test verifies that when FakeVEA is configured to fail upon start,
// MojoVEA::Initialize() causes a NotifyError().
TEST_F(MojoVideoEncodeAcceleratorTest, InitializeAndNotifyError) {
  CreateMojoVideoEncodeAccelerator(
      false /* will_fake_vea_initialization_succeed */);

  mojom::VideoEncodeAcceleratorClientPtr mojo_vea_client;
  auto mojo_vea_binding = mojo::MakeStrongBinding(
      base::MakeUnique<MockMojoVideoEncodeAcceleratorClient>(),
      mojo::MakeRequest(&mojo_vea_client));

  EXPECT_CALL(
      *static_cast<media::MockMojoVideoEncodeAcceleratorClient*>(
          mojo_vea_binding->impl()),
      NotifyError(
          mojom::VideoEncodeAccelerator::Error::PLATFORM_FAILURE_ERROR));

  const uint32_t kInitialBitrate = 100000u;
  mojo_vea()->Initialize(PIXEL_FORMAT_I420, kInputVisibleSize, H264PROFILE_MIN,
                         kInitialBitrate, std::move(mojo_vea_client));
  base::RunLoop().RunUntilIdle();

  mojo_vea_binding->Close();
}

// This test verifies that an any mojom::VEA method call (e.g. Encode(),
// UseOutputBitstreamBuffer() etc) before MojoVEA::Initialize() is ignored (we
// can't expect NotifyError()s since there's no mojo client registered).
TEST_F(MojoVideoEncodeAcceleratorTest, CallsBeforeInitializeAreIgnored) {
  CreateMojoVideoEncodeAccelerator();
  {
    const auto video_frame = VideoFrame::CreateBlackFrame(kInputVisibleSize);
    mojo_vea()->Encode(video_frame, true /* is_keyframe */,
                       base::Bind(&base::DoNothing));
    base::RunLoop().RunUntilIdle();
  }
  {
    const int32_t kBistreamBufferId = 17;
    const uint64_t kShMemSize = 10;
    auto handle = mojo::SharedBufferHandle::Create(kShMemSize);
    mojo_vea()->UseOutputBitstreamBuffer(kBistreamBufferId, std::move(handle));
    base::RunLoop().RunUntilIdle();
  }
  {
    const uint32_t kNewBitrate = 123123u;
    const uint32_t kNewFramerate = 321321u;
    mojo_vea()->RequestEncodingParametersChange(kNewBitrate, kNewFramerate);
    base::RunLoop().RunUntilIdle();
  }
}

}  // namespace media
