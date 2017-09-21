// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/filters/default_demuxer_factory.h"

#include "base/single_thread_task_runner.h"
#include "media/base/media_log.h"
#include "media/filters/chunk_demuxer.h"

#if !defined(MEDIA_DISABLE_FFMPEG)
#include "media/filters/ffmpeg_demuxer.h"
#endif

namespace media {

DefaultDemuxerFactory::DefaultDemuxerFactory(MediaLog* media_log)
    : media_log_(media_log) {}

DefaultDemuxerFactory::~DefaultDemuxerFactory() {}

std::unique_ptr<Demuxer> DefaultDemuxerFactory::CreateDemuxer(
    const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    DataSource* data_source,
    const Demuxer::EncryptedMediaInitDataCB& encrypted_media_init_data_cb,
    const Demuxer::MediaTracksUpdatedCB& media_tracks_updated_cb) {
#if !defined(MEDIA_DISABLE_FFMPEG)
  std::unique_ptr<Demuxer> demuxer(new FFmpegDemuxer(
      media_task_runner, data_source, encrypted_media_init_data_cb,
      media_tracks_updated_cb, media_log_));
#endif

  return demuxer;
}

std::unique_ptr<Demuxer> DefaultDemuxerFactory::CreateDemuxerSourceBuffer(
    const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    const base::Closure& open_cb,
    const base::Closure& progress_cb,
    const Demuxer::EncryptedMediaInitDataCB& encrypted_media_init_data_cb,
    media::SourceBuffer** source_buffer_out) {
  DCHECK(source_buffer_out);

  media::ChunkDemuxer* chunk_demuxer = new media::ChunkDemuxer(
      open_cb, progress_cb, encrypted_media_init_data_cb, media_log_);
  *source_buffer_out = chunk_demuxer;

  std::unique_ptr<Demuxer> demuxer(chunk_demuxer);

  return demuxer;
}

}  // namespace media
