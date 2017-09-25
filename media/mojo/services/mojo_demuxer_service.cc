// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_demuxer_service.h"

#include "base/bind_helpers.h"
#include "base/lazy_instance.h"
#include "media/base/data_buffer.h"
#include "media/base/demuxer_factory.h"
#include "media/base/media_tracks.h"
#include "media/mojo/common/media_type_converters.h"
#include "media/mojo/common/mojo_data_buffer_converter.h"
#include "media/mojo/services/mojo_data_source_adapter.h"
#include "media/mojo/services/mojo_demuxer_service_context.h"

namespace media {

MojoDemuxerService::MojoDemuxerService(
    base::WeakPtr<MojoDemuxerServiceContext> context,
    DemuxerFactory* demuxer_factory)
    : task_runner_(base::ThreadTaskRunnerHandle::Get()),
      context_(context),
      demuxer_factory_(demuxer_factory),
      data_source_(nullptr),
      demuxer_id_(MediaResource::kInvalidRemoteId),
      demuxer_(nullptr),
      source_buffer_(nullptr),
      weak_factory_(this) {
  DCHECK(context_);
  DCHECK(demuxer_factory_);
}

MojoDemuxerService::~MojoDemuxerService() {
  CHECK(task_runner_->BelongsToCurrentThread());
  if (demuxer_id_ == MediaResource::kInvalidRemoteId)
    return;

  if (context_)
    context_->UnregisterDemuxer(demuxer_id_);
}

media::Demuxer* MojoDemuxerService::GetDemuxerSourceBuffer(
    media::SourceBuffer** source_buffer_out) {
  CHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(source_buffer_out);

  *source_buffer_out = source_buffer_;

  return demuxer_.get();
}

media::Demuxer* MojoDemuxerService::GetDemuxer() {
  CHECK(task_runner_->BelongsToCurrentThread());
  return demuxer_.get();
}

void MojoDemuxerService::Initialize(
    mojom::DemuxerClientAssociatedPtrInfo client,
    int32_t demuxer_id,
    media::Demuxer::LoadType load_type,
    mojom::DataSourcePtr mojo_data_source,
    InitializeCallback callback) {
  CHECK(task_runner_->BelongsToCurrentThread());
  client_.Bind(std::move(client));

  std::unique_ptr<media::Demuxer> demuxer;

  if (load_type == media::Demuxer::LoadType::LoadTypeMediaSource) {
    demuxer = demuxer_factory_->CreateDemuxerSourceBuffer(
        base::ThreadTaskRunnerHandle::Get(),
        base::ThreadTaskRunnerHandle::Get(),
        base::Bind(&MojoDemuxerService::OnOpened, weak_factory_.GetWeakPtr()),
        base::Bind(&MojoDemuxerService::OnProgress, weak_factory_.GetWeakPtr()),
        base::Bind(&MojoDemuxerService::OnEncryptedMediaInitData,
                   weak_factory_.GetWeakPtr()),
        &source_buffer_);
  } else if (load_type == media::Demuxer::LoadType::LoadTypeURL) {
    CHECK(mojo_data_source);
    data_source_.reset(new MojoDataSourceAdapter(
        std::move(mojo_data_source),
        base::Bind(&MojoDemuxerService::OnDataSourceReady,
                   weak_factory_.GetWeakPtr(), base::Passed(&callback))));
    demuxer = demuxer_factory_->CreateDemuxer(
        nullptr, base::ThreadTaskRunnerHandle::Get(), data_source_.get(),
        base::Bind(&MojoDemuxerService::OnEncryptedMediaInitData,
                   weak_factory_.GetWeakPtr()),
        base::Bind(&MojoDemuxerService::OnMediaTracksUpdated,
                   weak_factory_.GetWeakPtr()));
  }

  if (!demuxer) {
    std::move(callback).Run(DEMUXER_ERROR_COULD_NOT_OPEN);
    return;
  }

  demuxer_ = std::move(demuxer);

  demuxer_id_ = demuxer_id;
  context_->RegisterDemuxer(demuxer_id_, this);

  if (load_type == media::Demuxer::LoadType::LoadTypeMediaSource) {
    demuxer_->Initialize(
        this,
        base::Bind(&MojoDemuxerService::OnDemuxerInitialized,
                   weak_factory_.GetWeakPtr(), base::Passed(&callback)),
        false);
  }
}

void MojoDemuxerService::OnDataSourceReady(InitializeCallback callback) {
  CHECK(task_runner_->BelongsToCurrentThread());
  demuxer_->Initialize(
      this,
      base::Bind(&MojoDemuxerService::OnDemuxerInitialized,
                 weak_factory_.GetWeakPtr(), base::Passed(&callback)),
      false);
}

void MojoDemuxerService::OnDemuxerInitialized(InitializeCallback callback,
                                              PipelineStatus status) {
  CHECK(task_runner_->BelongsToCurrentThread());
  std::move(callback).Run(status);
}

void MojoDemuxerService::StartWaitingForSeek(base::TimeDelta seek_time) {
  demuxer_->StartWaitingForSeek(seek_time);
}

void MojoDemuxerService::CancelPendingSeek(base::TimeDelta seek_time) {
  demuxer_->CancelPendingSeek(seek_time);
}

void MojoDemuxerService::Seek(base::TimeDelta time, SeekCallback callback) {
  demuxer_->Seek(
      time, base::Bind(&MojoDemuxerService::OnSeek, weak_factory_.GetWeakPtr(),
                       base::Passed(&callback)));
}

void MojoDemuxerService::OnSeek(SeekCallback callback, PipelineStatus status) {
  std::move(callback).Run(status);
}

void MojoDemuxerService::Stop() {
  demuxer_->Stop();
}

void MojoDemuxerService::AbortPendingReads() {
  demuxer_->AbortPendingReads();
}

void MojoDemuxerService::GetStartTime(GetStartTimeCallback callback) {
  base::TimeDelta start_time = demuxer_->GetStartTime();
  std::move(callback).Run(start_time);
}

void MojoDemuxerService::GetTimelineOffset(GetTimelineOffsetCallback callback) {
  base::Time timeline_offset = demuxer_->GetTimelineOffset();
  std::move(callback).Run(timeline_offset);
}

void MojoDemuxerService::GetMemoryUsage(GetMemoryUsageCallback callback) {
  int64_t memory_usage = demuxer_->GetMemoryUsage();
  std::move(callback).Run(memory_usage);
}

void MojoDemuxerService::OnEnabledAudioTracksChanged(
    const std::vector<MediaTrack::Id>& track_ids,
    base::TimeDelta current_time) {
  demuxer_->OnEnabledAudioTracksChanged(track_ids, current_time);
}
void MojoDemuxerService::OnSelectedVideoTrackChanged(
    const base::Optional<MediaTrack::Id>& selected_track_id,
    base::TimeDelta current_time) {
  demuxer_->OnSelectedVideoTrackChanged(selected_track_id, current_time);
}

void MojoDemuxerService::GetAllStreams(GetAllStreamsCallback callback) {
  std::vector<AudioDecoderConfig> audio_decoder_configs;
  std::vector<VideoDecoderConfig> video_decoder_configs;

  const auto& streams = demuxer_->GetAllStreams();

  for (auto* stream : streams) {
    if (stream->type() == DemuxerStream::Type::AUDIO)
      audio_decoder_configs.push_back(stream->audio_decoder_config());
    else if (stream->type() == DemuxerStream::Type::VIDEO)
      video_decoder_configs.push_back(stream->video_decoder_config());
  }

  std::move(callback).Run(std::move(audio_decoder_configs),
                          std::move(video_decoder_configs));
}

void MojoDemuxerService::OnEncryptedMediaInitData(
    EmeInitDataType init_data_type,
    const std::vector<uint8_t>& init_data) {
  CHECK(task_runner_->BelongsToCurrentThread());
  client_->OnEncryptedMediaInitData(init_data_type, init_data);
}

void MojoDemuxerService::OnMediaTracksUpdated(
    std::unique_ptr<MediaTracks> tracks) {
  CHECK(task_runner_->BelongsToCurrentThread());
  mojom::MediaTracksPtr mojo_tracks = mojom::MediaTracks::From(*tracks.get());
  client_->OnMediaTracksUpdated(std::move(mojo_tracks));
}

void MojoDemuxerService::OnBufferedTimeRangesChanged(
    const Ranges<base::TimeDelta>& ranges) {
  CHECK(task_runner_->BelongsToCurrentThread());
  mojom::RangesTimeDeltaPtr mojo_ranges = mojom::RangesTimeDelta::From(ranges);
  client_->OnBufferedTimeRangesChanged(std::move(mojo_ranges));
}

void MojoDemuxerService::SetDuration(base::TimeDelta duration) {
  CHECK(task_runner_->BelongsToCurrentThread());
  client_->OnSetDuration(duration);
}

void MojoDemuxerService::OnDemuxerError(PipelineStatus error) {
  CHECK(task_runner_->BelongsToCurrentThread());
  client_->OnDemuxerError(error);
}

void MojoDemuxerService::OnOpened() {
  CHECK(task_runner_->BelongsToCurrentThread());
  client_->OnOpened();
}

void MojoDemuxerService::OnProgress() {
  CHECK(task_runner_->BelongsToCurrentThread());
  client_->OnProgress();
}

void MojoDemuxerService::AddTextStream(DemuxerStream* text_stream,
                                       const TextTrackConfig& config) {
  NOTIMPLEMENTED();
}

void MojoDemuxerService::RemoveTextStream(DemuxerStream* text_stream) {
  NOTIMPLEMENTED();
}

}  // namespace media
