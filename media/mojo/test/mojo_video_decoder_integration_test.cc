// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <vector>

#include <stdint.h>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback_forward.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_task_environment.h"
#include "base/time/time.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "media/base/decode_status.h"
#include "media/base/decoder_buffer.h"
#include "media/base/encryption_scheme.h"
#include "media/base/gmock_callback_support.h"
#include "media/base/media_util.h"
#include "media/base/mock_media_log.h"
#include "media/base/video_codecs.h"
#include "media/base/video_decoder.h"
#include "media/base/video_frame.h"
#include "media/base/video_rotation.h"
#include "media/base/video_types.h"
#include "media/mojo/clients/mojo_video_decoder.h"
#include "media/mojo/services/mojo_media_client.h"
#include "media/mojo/services/mojo_video_decoder_service.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Mock;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::StrictMock;

namespace media {

namespace {

// A mock VideoDecoder with helpful default functionality.
class MockVideoDecoder : public VideoDecoder {
 public:
  MockVideoDecoder() {
    // Treat const getters like a NiceMock.
    EXPECT_CALL(*this, GetDisplayName())
        .WillRepeatedly(Return("MockVideoDecoder"));
    EXPECT_CALL(*this, NeedsBitstreamConversion())
        .WillRepeatedly(Return(false));
    EXPECT_CALL(*this, CanReadWithoutStalling()).WillRepeatedly(Return(true));
    EXPECT_CALL(*this, GetMaxDecodeRequests()).WillRepeatedly(Return(1));

    // For regular methods, only configure a default action.
    ON_CALL(*this, DoInitialize(_)).WillByDefault(RunCallback<0>(true));
    ON_CALL(*this, Decode(_, _))
        .WillByDefault(Invoke(this, &MockVideoDecoder::DoDecode));
    ON_CALL(*this, Reset(_)).WillByDefault(RunCallback<0>());
  }

  // Re-declare as public.
  ~MockVideoDecoder() override {}

  // media::VideoDecoder implementation
  MOCK_CONST_METHOD0(GetDisplayName, std::string());

  // Initialize() records values before delegating to the mock method.
  void Initialize(const VideoDecoderConfig& config,
                  bool low_delay,
                  CdmContext* cdm_context,
                  const InitCB& init_cb,
                  const OutputCB& output_cb) override {
    config_ = config;
    output_cb_ = output_cb;
    DoInitialize(init_cb);
  }

  MOCK_METHOD2(Decode,
               void(const scoped_refptr<DecoderBuffer>& buffer,
                    const DecodeCB&));
  MOCK_METHOD1(Reset, void(const base::Closure&));
  MOCK_CONST_METHOD0(NeedsBitstreamConversion, bool());
  MOCK_CONST_METHOD0(CanReadWithoutStalling, bool());
  MOCK_CONST_METHOD0(GetMaxDecodeRequests, int());

  // Mock helpers.
  MOCK_METHOD1(DoInitialize, void(const InitCB&));

  // Returns an output frame immediately.
  void DoDecode(const scoped_refptr<DecoderBuffer>& buffer,
                const DecodeCB& decode_cb) {
    if (!buffer->end_of_stream()) {
      gpu::MailboxHolder mailbox_holders[VideoFrame::kMaxPlanes];
      mailbox_holders[0].mailbox.name[0] = 1;
      output_cb_.Run(VideoFrame::WrapNativeTextures(
          PIXEL_FORMAT_ARGB, mailbox_holders,
          // TODO(sandersd): Hook release callbacks.
          VideoFrame::ReleaseMailboxCB(), config_.coded_size(),
          config_.visible_rect(), config_.natural_size(), buffer->timestamp()));
    }
    // |decode_cb| must not be called from the same stack.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(decode_cb, DecodeStatus::OK));
  }

 private:
  // Destructing a std::unique_ptr<VideoDecoder>(this) is a no-op.
  // TODO(sandersd): After this, any method call is an error. Implement checks
  // for that.
  void Destroy() override { DVLOG(1) << __func__ << "(): Ignored"; }

  VideoDecoderConfig config_;
  OutputCB output_cb_;

  DISALLOW_COPY_AND_ASSIGN(MockVideoDecoder);
};

// Proxies CreateVideoDecoder() to a callback.
class FakeMojoMediaClient : public MojoMediaClient {
 public:
  using CreateVideoDecoderCB =
      base::Callback<std::unique_ptr<VideoDecoder>(MediaLog*)>;

  FakeMojoMediaClient(CreateVideoDecoderCB create_video_decoder_cb)
      : create_video_decoder_cb_(std::move(create_video_decoder_cb)) {}

  std::unique_ptr<VideoDecoder> CreateVideoDecoder(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      MediaLog* media_log,
      mojom::CommandBufferIdPtr command_buffer_id,
      OutputWithReleaseMailboxCB output_cb,
      RequestOverlayInfoCB request_overlay_info_cb) override {
    return create_video_decoder_cb_.Run(media_log);
  }

 private:
  CreateVideoDecoderCB create_video_decoder_cb_;

  DISALLOW_COPY_AND_ASSIGN(FakeMojoMediaClient);
};

}  // namespace

class MojoVideoDecoderIntegrationTest : public ::testing::Test {
 public:
  MojoVideoDecoderIntegrationTest()
      : mojo_media_client_(
            base::Bind(&MojoVideoDecoderIntegrationTest::CreateVideoDecoder,
                       base::Unretained(this))) {
    mojom::VideoDecoderPtr remote_video_decoder;
    binding_ = mojo::MakeStrongBinding(
        std::make_unique<MojoVideoDecoderService>(&mojo_media_client_, nullptr),
        mojo::MakeRequest(&remote_video_decoder));
    client = std::make_unique<MojoVideoDecoder>(
        base::ThreadTaskRunnerHandle::Get(), nullptr, &client_media_log,
        std::move(remote_video_decoder), RequestOverlayInfoCB());
  }

  void TearDown() override {
    if (client) {
      client.reset();
      RunUntilIdle();
    }
  }

  void RunUntilIdle() { scoped_task_environment_.RunUntilIdle(); }

  bool Initialize() {
    bool result = false;

    EXPECT_CALL(*decoder, DoInitialize(_));

    StrictMock<base::MockCallback<VideoDecoder::InitCB>> init_cb;
    EXPECT_CALL(init_cb, Run(_)).WillOnce(SaveArg<0>(&result));

    client->Initialize(CreateVP9Config(), false, nullptr, init_cb.Get(),
                       output_cb.Get());
    RunUntilIdle();

    return result;
  }

  DecodeStatus Decode(const scoped_refptr<DecoderBuffer>& buffer) {
    DecodeStatus result = DecodeStatus::DECODE_ERROR;

    EXPECT_CALL(*decoder, Decode(_, _));

    StrictMock<base::MockCallback<VideoDecoder::DecodeCB>> decode_cb;
    EXPECT_CALL(decode_cb, Run(_)).WillOnce(SaveArg<0>(&result));

    client->Decode(buffer, decode_cb.Get());
    RunUntilIdle();

    return result;
  }

  VideoDecoderConfig CreateVP9Config() {
    return VideoDecoderConfig(
        kCodecVP9,  // must not be kUnknownVideoCodec
        VP9PROFILE_MIN,
        PIXEL_FORMAT_ARGB,  // must not be PIXEL_FORMAT_UNKNOWN
        COLOR_SPACE_HD_REC709, VIDEO_ROTATION_0,
        gfx::Size(1920, 1088),  // must not be empty (or too large)
        gfx::Rect(1920, 1080),  // must not be empty (or too large)
        gfx::Size(1920, 1080),  // must not be empty (or too large)
        EmptyExtraData(), EncryptionScheme());
  }

  scoped_refptr<DecoderBuffer> CreateKeyframe(int64_t timestamp_ms) {
    std::vector<uint8_t> data = {1, 2, 3, 4};
    scoped_refptr<DecoderBuffer> buffer =
        DecoderBuffer::CopyFrom(data.data(), data.size());
    buffer->set_timestamp(base::TimeDelta::FromMilliseconds(timestamp_ms));
    buffer->set_duration(base::TimeDelta::FromMilliseconds(10));
    buffer->set_is_key_frame(true);
    return buffer;
  }

  // Callback that |client| will deliver VideoFrames to.
  StrictMock<base::MockCallback<VideoDecoder::OutputCB>> output_cb;

  // MojoVideoDecoder (client) under test.
  std::unique_ptr<MojoVideoDecoder> client;

  // MediaLog that |client| will deliver log events to.
  StrictMock<MockMediaLog> client_media_log;

  // VideoDecoder (impl used by service) under test.
  std::unique_ptr<MockVideoDecoder> decoder =
      std::make_unique<StrictMock<MockVideoDecoder>>();

  // MediaLog that the service has provided to |decoder|. This should be proxied
  // to |client_media_log|.
  MediaLog* decoder_media_log = nullptr;

 private:
  // Passes |decoder| to the service.
  std::unique_ptr<VideoDecoder> CreateVideoDecoder(MediaLog* media_log) {
    DCHECK(!decoder_media_log);
    decoder_media_log = media_log;
    // Since MockVideoDecoder::Destroy() is a no-op, this doesn't actually
    // transfer ownership.
    return std::unique_ptr<VideoDecoder>(decoder.get());
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;

  // Provides |decoder| to the service.
  FakeMojoMediaClient mojo_media_client_;

  // Owns the service, bound to |client|.
  mojo::StrongBindingPtr<mojom::VideoDecoder> binding_;

  DISALLOW_COPY_AND_ASSIGN(MojoVideoDecoderIntegrationTest);
};

TEST_F(MojoVideoDecoderIntegrationTest, CreateAndDestroy) {}

TEST_F(MojoVideoDecoderIntegrationTest, Initialize) {
  ASSERT_TRUE(Initialize());
  EXPECT_EQ(client->GetDisplayName(), "MojoVideoDecoder");
  EXPECT_EQ(client->NeedsBitstreamConversion(), false);
  EXPECT_EQ(client->CanReadWithoutStalling(), true);
  EXPECT_EQ(client->GetMaxDecodeRequests(), 1);
}

TEST_F(MojoVideoDecoderIntegrationTest, Decode) {
  ASSERT_TRUE(Initialize());

  EXPECT_CALL(output_cb, Run(_));
  ASSERT_EQ(Decode(CreateKeyframe(0)), DecodeStatus::OK);
  Mock::VerifyAndClearExpectations(&output_cb);

  ASSERT_EQ(Decode(DecoderBuffer::CreateEOSBuffer()), DecodeStatus::OK);
}

}  // namespace media
