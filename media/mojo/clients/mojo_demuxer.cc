// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/clients/mojo_demuxer.h"

#include "base/callback_helpers.h"
#include "media/base/media_tracks.h"
#include "media/mojo/clients/mojo_data_source_impl.h"
#include "media/mojo/common/media_type_converters.h"

using base::TimeDelta;

namespace media {

MojoDemuxer::MojoDemuxer(
    int32_t remote_id,
    Demuxer::LoadType load_type,
    DataSource* data_source,
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    mojom::DemuxerPtr mojo_demuxer,
    const base::Closure& open_cb,
    const base::Closure& progress_cb,
    const Demuxer::EncryptedMediaInitDataCB& encrypted_media_init_data_cb,
    const Demuxer::MediaTracksUpdatedCB& media_tracks_updated_cb)
    : media_task_runner_(media_task_runner),
      has_audio_(false),
      has_video_(false),
      load_type_(load_type),
      mojo_data_source_(nullptr),
      data_source_(data_source),
      open_cb_(open_cb),
      progress_cb_(progress_cb),
      encrypted_media_init_data_cb_(encrypted_media_init_data_cb),
      media_tracks_updated_cb_(media_tracks_updated_cb),
      remote_demuxer_info_(mojo_demuxer.PassInterface()),
      client_binding_(this),
      weak_factory_(this) {
  DCHECK(!open_cb_.is_null());
  DCHECK(remote_id_ != MediaResource::kInvalidRemoteId);
  remote_id_ = remote_id;
}

MojoDemuxer::~MojoDemuxer() {
  // Demuxer is always destroyed in the main thread.
  CHECK(!media_task_runner_->BelongsToCurrentThread());
  CHECK(!remote_demuxer_);
  CHECK(!mojo_data_source_);
}

MediaResource::Type MojoDemuxer::GetType() const {
  return MediaResource::Type::REMOTE;
}

// Should never be called since MediaResource::Type is REMOTE.
std::vector<DemuxerStream*> MojoDemuxer::GetAllStreams() {
  NOTREACHED();
  return std::vector<DemuxerStream*>();
}

// Should never be called since MediaResource::Type is REMOTE.
void MojoDemuxer::SetStreamStatusChangeCB(const StreamStatusChangeCB& cb) {
  NOTREACHED();
}

bool MojoDemuxer::HasAudio() const {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  return has_audio_;
}

bool MojoDemuxer::HasVideo() const {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  return has_video_;
}

std::string MojoDemuxer::GetDisplayName() const {
  return "MojoDemuxer";
}

void MojoDemuxer::Initialize(DemuxerHost* host,
                             const PipelineStatusCB& status_cb,
                             bool enable_text_tracks) {
  CHECK(media_task_runner_->BelongsToCurrentThread());

  host_ = host;
  status_cb_ = status_cb;

  // Bind |mojo_demuxer_| to the |media_task_runner_|.
  if (!remote_demuxer_.is_bound())
    remote_demuxer_.Bind(std::move(remote_demuxer_info_));

  remote_demuxer_.set_connection_error_handler(
      base::Bind(&MojoDemuxer::OnConnectionError, weak_factory_.GetWeakPtr()));

  mojom::DemuxerClientAssociatedPtrInfo client_ptr_info;
  client_binding_.Bind(mojo::MakeRequest(&client_ptr_info));

  mojom::DataSourcePtr data_source_ptr;
  if (data_source_) {
    mojo_data_source_.reset(
        new MojoDataSourceImpl(data_source_, MakeRequest(&data_source_ptr)));
    mojo_data_source_->set_connection_error_handler(base::Bind(
        &MojoDemuxer::OnDataSourceConnectionError, weak_factory_.GetWeakPtr()));
  }

  remote_demuxer_->Initialize(
      std::move(client_ptr_info), GetRemoteId(), load_type_,
      std::move(data_source_ptr),
      base::Bind(&MojoDemuxer::OnInitialized, weak_factory_.GetWeakPtr()));
}

void MojoDemuxer::OnDataSourceConnectionError() {
  CHECK(media_task_runner_->BelongsToCurrentThread());

  mojo_data_source_.reset();
}

void MojoDemuxer::OnInitialized(PipelineStatus status) {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  status_cb_.Run(status);
}

void MojoDemuxer::StartWaitingForSeek(base::TimeDelta seek_time) {
  CHECK(!media_task_runner_->BelongsToCurrentThread());

  remote_demuxer_->StartWaitingForSeek(seek_time);
}

void MojoDemuxer::CancelPendingSeek(base::TimeDelta seek_time) {
  if (media_task_runner_->BelongsToCurrentThread())
    remote_demuxer_->CancelPendingSeek(seek_time);
  else {
    base::Bind(&MojoDemuxer::CancelPendingSeek, weak_factory_.GetWeakPtr(),
               seek_time);
  }
}

void MojoDemuxer::Seek(base::TimeDelta time,
                       const PipelineStatusCB& status_cb) {
  CHECK(media_task_runner_->BelongsToCurrentThread());

  remote_demuxer_->Seek(
      time,
      base::Bind(&MojoDemuxer::OnSeek, weak_factory_.GetWeakPtr(), status_cb));
}

void MojoDemuxer::OnSeek(const PipelineStatusCB& status_cb,
                         PipelineStatus status) {
  media_task_runner_->PostTask(FROM_HERE, base::Bind(status_cb, status));
}

void MojoDemuxer::Stop() {
  CHECK(media_task_runner_->BelongsToCurrentThread());

  remote_demuxer_->Stop();

  // Unbind in the bound thread.
  remote_demuxer_.reset();
  mojo_data_source_.reset();
}

void MojoDemuxer::AbortPendingReads() {
  CHECK(media_task_runner_->BelongsToCurrentThread());

  remote_demuxer_->AbortPendingReads();
}

base::TimeDelta MojoDemuxer::GetStartTime() const {
  CHECK(media_task_runner_->BelongsToCurrentThread());

  base::TimeDelta start_time;
  remote_demuxer_->GetStartTime(&start_time);

  return start_time;
}
base::Time MojoDemuxer::GetTimelineOffset() const {
  CHECK(media_task_runner_->BelongsToCurrentThread());

  base::Time timeline_offset;
  remote_demuxer_->GetTimelineOffset(&timeline_offset);

  return timeline_offset;
}

int64_t MojoDemuxer::GetMemoryUsage() const {
  CHECK(media_task_runner_->BelongsToCurrentThread());

  int64_t memory_usage = 0;
  remote_demuxer_->GetMemoryUsage(&memory_usage);

  return memory_usage;
}

void MojoDemuxer::OnEnabledAudioTracksChanged(
    const std::vector<MediaTrack::Id>& track_ids,
    base::TimeDelta current_time) {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  remote_demuxer_->OnEnabledAudioTracksChanged(track_ids, current_time);
}
void MojoDemuxer::OnSelectedVideoTrackChanged(
    base::Optional<MediaTrack::Id> selected_track_id,
    base::TimeDelta current_time) {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  remote_demuxer_->OnSelectedVideoTrackChanged(selected_track_id, current_time);
}

void MojoDemuxer::OnConnectionError() {
  CHECK(media_task_runner_->BelongsToCurrentThread());

  host_->OnDemuxerError(PIPELINE_ERROR_READ);
}

void MojoDemuxer::OnEncryptedMediaInitData(
    EmeInitDataType init_data_type,
    const std::vector<uint8_t>& init_data) {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  encrypted_media_init_data_cb_.Run(init_data_type, init_data);
}

void MojoDemuxer::OnMediaTracksUpdated(mojom::MediaTracksPtr mojo_tracks) {
  CHECK(media_task_runner_->BelongsToCurrentThread());

  std::unique_ptr<MediaTracks> tracks(
      mojo_tracks.To<std::unique_ptr<MediaTracks>>());

  // Useful for PipelineMetadata
  has_audio_ = false;
  has_video_ = false;
  for (const auto& track : tracks->tracks()) {
    if (track->type() == MediaTrack::Type::Audio)
      has_audio_ = true;
    else if (track->type() == MediaTrack::Type::Video)
      has_video_ = true;
  }

  media_tracks_updated_cb_.Run(std::move(tracks));
}

void MojoDemuxer::OnBufferedTimeRangesChanged(
    mojom::RangesTimeDeltaPtr mojo_ranges) {
  CHECK(media_task_runner_->BelongsToCurrentThread());

  Ranges<base::TimeDelta> ranges = mojo_ranges.To<Ranges<base::TimeDelta>>();
  host_->OnBufferedTimeRangesChanged(ranges);
}

void MojoDemuxer::OnSetDuration(base::TimeDelta duration) {
  CHECK(media_task_runner_->BelongsToCurrentThread());

  host_->SetDuration(duration);
}

void MojoDemuxer::OnOpened() {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  base::ResetAndReturn(&open_cb_).Run();
}

void MojoDemuxer::OnProgress() {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  progress_cb_.Run();
}

void MojoDemuxer::OnDemuxerError(PipelineStatus error) {
  CHECK(media_task_runner_->BelongsToCurrentThread());

  host_->OnDemuxerError(error);
}

}  // namespace media
