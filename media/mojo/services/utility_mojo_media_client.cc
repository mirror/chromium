// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/utility_mojo_media_client.h"

#include "base/threading/thread_task_runner_handle.h"
#include "media/base/media.h"
#include "media/base/video_decoder.h"
#include "media/mojo/common/mojo_shared_buffer_video_frame.h"
#include "media/mojo/common/mojo_video_frame_provider.h"

#if !defined(DISABLE_FFMPEG_VIDEO_DECODERS)
#include "media/filters/ffmpeg_video_decoder.h"
#endif

namespace media {

UtilityMojoMediaClient::UtilityMojoMediaClient() {}

UtilityMojoMediaClient::~UtilityMojoMediaClient() {}

void UtilityMojoMediaClient::Initialize(
    service_manager::Connector* connector,
    service_manager::ServiceContextRefFactory* context_ref_factory) {
  InitializeMediaLibrary();
}

std::unique_ptr<VideoDecoder> UtilityMojoMediaClient::CreateVideoDecoder(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    MediaLog* media_log,
    mojom::CommandBufferIdPtr command_buffer_id,
    OutputWithReleaseMailboxCB output_cb) {
  std::unique_ptr<VideoDecoder> decoder;

#if !defined(DISABLE_FFMPEG_VIDEO_DECODERS)
  std::unique_ptr<MojoVideoFrameProvider> video_frame_provider(
      new MojoVideoFrameProvider());
  decoder.reset(
      new FFmpegVideoDecoder(media_log, std::move(video_frame_provider)));
#endif

  return decoder;
}

}  // namespace media
