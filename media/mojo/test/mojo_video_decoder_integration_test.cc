// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/scoped_task_environment.h"
#include "base/time/time.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "media/base/decode_status.h"
#include "media/base/decoder_buffer.h"
#include "media/base/encryption_scheme.h"
#include "media/base/media_log.h"
#include "media/base/media_util.h"
#include "media/base/video_codecs.h"
#include "media/base/video_decoder.h"
#include "media/base/video_frame.h"
#include "media/base/video_rotation.h"
#include "media/base/video_types.h"
#include "media/mojo/clients/mojo_video_decoder.h"
#include "media/mojo/services/mojo_media_client.h"
#include "media/mojo/services/mojo_video_decoder_service.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace media {

namespace {

// TODO(sandersd): Use MockMediaLog?
// TODO(sandersd): Create integration test for MojoMediaLog.
class FakeMediaLog : public MediaLog {
 public:
  FakeMediaLog() = default;
  ~FakeMediaLog() override = default;

  // media::MediaLog implementation
  void AddEvent(std::unique_ptr<media::MediaLogEvent> event) override {
    DVLOG(3) << __func__;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeMediaLog);
};

// TODO(sandersd): This should probably be a mock.
class FakeVideoDecoder : public VideoDecoder {
 public:
  FakeVideoDecoder() = default;

  // Re-declaring public to avoid configuring default_delete.
  ~FakeVideoDecoder() override = default;

  // media::VideoDecoder implementation
  std::string GetDisplayName() const override { return "FakeVideoDecoder"; }

  void Initialize(const VideoDecoderConfig& config,
                  bool low_delay,
                  CdmContext* cdm_context,
                  const InitCB& init_cb,
                  const OutputCB& output_cb) override {
    DVLOG(1) << __func__;
    config_ = config;
    output_cb_ = output_cb;
    init_cb.Run(true);
  }

  void Decode(const scoped_refptr<DecoderBuffer>& buffer,
              const DecodeCB& decode_cb) override {
    DVLOG(2) << __func__;
    if (buffer->end_of_stream()) {
      output_cb_.Run(VideoFrame::CreateEOSFrame());
    } else {
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

  void Reset(const base::Closure& reset_cb) override {
    DVLOG(1) << __func__;
    reset_cb.Run();
  }

 private:
  VideoDecoderConfig config_;
  OutputCB output_cb_;

  DISALLOW_COPY_AND_ASSIGN(FakeVideoDecoder);
};

class FakeMojoMediaClient : public MojoMediaClient {
 public:
  FakeMojoMediaClient() = default;

  std::unique_ptr<VideoDecoder> CreateVideoDecoder(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      MediaLog* media_log,
      mojom::CommandBufferIdPtr command_buffer_id,
      OutputWithReleaseMailboxCB output_cb,
      RequestOverlayInfoCB request_overlay_info_cb) override {
    DVLOG(3) << __func__;
    return std::make_unique<FakeVideoDecoder>();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeMojoMediaClient);
};

}  // namespace

class MojoVideoDecoderIntegrationTest : public ::testing::Test {
 public:
  MojoVideoDecoderIntegrationTest() {}

  void SetUp() override {
    mojom::VideoDecoderPtr remote_video_decoder;
    mojo_video_decoder_service_binding_ = mojo::MakeStrongBinding(
        std::make_unique<MojoVideoDecoderService>(&mojo_media_client_),
        mojo::MakeRequest(&remote_video_decoder));
    mojo_video_decoder_ = std::make_unique<MojoVideoDecoder>(
        base::ThreadTaskRunnerHandle::Get(),
        nullptr,  // gpu_factories
        &media_log_, std::move(remote_video_decoder), RequestOverlayInfoCB());
  }

  bool Initialize() {
    VideoDecoderConfig config(
        kCodecVP9,  // must not be kUnknownVideoCodec
        VP9PROFILE_MIN,
        PIXEL_FORMAT_ARGB,  // must not be PIXEL_FORMAT_UNKNOWN
        COLOR_SPACE_HD_REC709, VIDEO_ROTATION_0,
        gfx::Size(1920, 1088),  // must not be empty (or too large)
        gfx::Rect(1920, 1080),  // must not be empty (or too large)
        gfx::Size(1920, 1080),  // must not be empty (or too large)
        EmptyExtraData(), EncryptionScheme());
    mojo_video_decoder_->Initialize(
        config,
        false,    // low_delay
        nullptr,  // cdm_context
        base::Bind(&MojoVideoDecoderIntegrationTest::OnInitializeDone,
                   base::Unretained(this)),
        base::Bind(&MojoVideoDecoderIntegrationTest::OnOutput,
                   base::Unretained(this)));
    init_result_ = false;
    scoped_task_environment_.RunUntilIdle();
    return init_result_;
  }

  void Decode(const scoped_refptr<DecoderBuffer>& buffer) {
    mojo_video_decoder_->Decode(
        buffer, base::Bind(&MojoVideoDecoderIntegrationTest::OnDecodeDone,
                           base::Unretained(this)));
    scoped_task_environment_.RunUntilIdle();
  }

  void Reset() {
    mojo_video_decoder_->Reset(base::Bind(
        &MojoVideoDecoderIntegrationTest::OnResetDone, base::Unretained(this)));
    scoped_task_environment_.RunUntilIdle();
  }

  MojoVideoDecoder* Client() { return mojo_video_decoder_.get(); }

 private:
  void OnInitializeDone(bool success) {
    DVLOG(2) << __func__ << "(" << success << ")";
    init_result_ = success;
  }

  void OnDecodeDone(DecodeStatus status) {
    DVLOG(2) << __func__ << "(" << status << ")";
  }

  void OnOutput(const scoped_refptr<VideoFrame>& frame) {
    DVLOG(2) << __func__;
  }

  void OnResetDone() { DVLOG(2) << __func__; }

  base::test::ScopedTaskEnvironment scoped_task_environment_;

  FakeMojoMediaClient mojo_media_client_;
  FakeMediaLog media_log_;

  mojo::StrongBindingPtr<mojom::VideoDecoder>
      mojo_video_decoder_service_binding_;
  std::unique_ptr<MojoVideoDecoder> mojo_video_decoder_;

  bool init_result_;

  DISALLOW_COPY_AND_ASSIGN(MojoVideoDecoderIntegrationTest);
};

TEST_F(MojoVideoDecoderIntegrationTest, CreateAndDestroy) {}

TEST_F(MojoVideoDecoderIntegrationTest, Initialize) {
  ASSERT_TRUE(Initialize());
  EXPECT_EQ(Client()->GetDisplayName(), "MojoVideoDecoder");
  EXPECT_EQ(Client()->NeedsBitstreamConversion(), false);
  EXPECT_EQ(Client()->CanReadWithoutStalling(), true);
  EXPECT_EQ(Client()->GetMaxDecodeRequests(), 1);
}

TEST_F(MojoVideoDecoderIntegrationTest, Decode) {
  ASSERT_TRUE(Initialize());

  scoped_refptr<DecoderBuffer> buffer =
      base::MakeRefCounted<DecoderBuffer>(1024);
  buffer->set_timestamp(base::TimeDelta::FromMilliseconds(0));
  buffer->set_duration(base::TimeDelta::FromMilliseconds(10));
  buffer->set_is_key_frame(true);
  Decode(buffer);

  Decode(DecoderBuffer::CreateEOSBuffer());
}

}  // namespace media
