// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/utility_mojo_media_client.h"

#include "base/threading/thread_task_runner_handle.h"
#include "media/audio/null_audio_sink.h"
#include "media/base/media.h"
#include "media/base/video_decoder.h"
#include "media/filters/default_demuxer_factory.h"
#include "media/mojo/common/mojo_shared_buffer_video_frame.h"
#include "media/mojo/common/mojo_video_frame_provider.h"
#include "media/mojo/common/mojo_video_frame_provider_factory.h"
#include "media/mojo/services/mojo_audio_renderer_sink_adapter.h"
#include "media/mojo/services/mojo_video_renderer_sink_adapter.h"
#include "media/renderers/default_renderer_factory.h"

#if !defined(DISABLE_FFMPEG_VIDEO_DECODERS)
#include "media/filters/ffmpeg_video_decoder.h"
#endif

#if !defined(MEDIA_DISABLE_LIBVPX)
#include "media/filters/vpx_video_decoder.h"
#endif

namespace media {

UtilityMojoMediaClient::UtilityMojoMediaClient(
    scoped_refptr<base::SingleThreadTaskRunner> utility_task_runner)
    : utility_task_runner_(std::move(utility_task_runner)) {}

UtilityMojoMediaClient::~UtilityMojoMediaClient() {}

void UtilityMojoMediaClient::Initialize(
    service_manager::Connector* connector,
    service_manager::ServiceContextRefFactory* context_ref_factory) {
  InitializeMediaLibrary();
}

scoped_refptr<AudioRendererSink>
UtilityMojoMediaClient::CreateAudioRendererSink(
    const std::string& audio_device_id) {
  return new MojoAudioRendererSinkAdapter(audio_device_id);
}

std::unique_ptr<VideoRendererSink>
UtilityMojoMediaClient::CreateVideoRendererSink(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) {
  return base::MakeUnique<MojoVideoRendererSinkAdapter>(task_runner);
}

std::unique_ptr<VideoDecoder> UtilityMojoMediaClient::CreateVideoDecoder(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    MediaLog* media_log,
    mojom::CommandBufferIdPtr command_buffer_id,
    OutputWithReleaseMailboxCB output_cb,
    RequestOverlayInfoCB request_overlay_info_cb,
    const std::string& decoder_name) {
#if !defined(MEDIA_DISABLE_LIBVPX)
  if (decoder_name == "VpxVideoDecoder") {
    std::unique_ptr<VideoDecoder> decoder(
        new VpxVideoDecoder(base::MakeUnique<MojoVideoFrameProvider>()));
    CHECK_EQ(decoder_name, decoder->GetDisplayName());
    return decoder;
  }
#endif

#if !defined(DISABLE_FFMPEG_VIDEO_DECODERS)
  if (decoder_name == "FFmpegVideoDecoder") {
    std::unique_ptr<VideoDecoder> decoder(new FFmpegVideoDecoder(
        media_log, base::MakeUnique<MojoVideoFrameProvider>()));
    CHECK_EQ(decoder_name, decoder->GetDisplayName());
    return decoder;
  }
#endif

  return nullptr;
}

std::unique_ptr<RendererFactory> UtilityMojoMediaClient::CreateRendererFactory(
    MediaLog* media_log) {
  std::unique_ptr<VideoFrameProviderFactory> video_frame_provider_factory(
      new MojoVideoFrameProviderFactory());
  return base::MakeUnique<DefaultRendererFactory>(
      media_log, nullptr, DefaultRendererFactory::GetGpuFactoriesCB(),
      std::move(video_frame_provider_factory));
}

std::unique_ptr<DemuxerFactory> UtilityMojoMediaClient::CreateDemuxerFactory(
    MediaLog* media_log) {
  return base::MakeUnique<DefaultDemuxerFactory>(media_log);
}

}  // namespace media
