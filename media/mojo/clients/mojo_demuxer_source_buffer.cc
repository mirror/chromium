// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/clients/mojo_demuxer_source_buffer.h"

#include "base/callback_helpers.h"
#include "media/base/data_buffer.h"
#include "media/base/media_tracks.h"
#include "media/mojo/common/media_type_converters.h"
#include "media/mojo/common/mojo_data_buffer_converter.h"

using base::TimeDelta;

namespace media {

MojoDemuxerSourceBuffer::MojoDemuxerSourceBuffer(
    int32_t id,
    const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    mojom::DemuxerPtr mojo_demuxer,
    mojom::SourceBufferPtr mojo_source_buffer,
    const base::Closure& open_cb,
    const base::Closure& progress_cb,
    const Demuxer::EncryptedMediaInitDataCB& encrypted_media_init_data_cb)
    : MojoDemuxer(id,
                  Demuxer::LoadType::LoadTypeMediaSource,
                  nullptr,
                  media_task_runner,
                  std::move(mojo_demuxer),
                  open_cb,
                  progress_cb,
                  encrypted_media_init_data_cb,
                  base::Bind(&MojoDemuxerSourceBuffer::OnMediaTracksDoNothing,
                             base::Unretained(this))),
      main_task_runner_(main_task_runner),
      remote_source_buffer_(std::move(mojo_source_buffer)),
      client_binding_(this),
      duration_(0),
      weak_factory_(this) {
  CHECK(main_task_runner_->BelongsToCurrentThread());
  DCHECK(remote_source_buffer_);

  remote_source_buffer_.set_connection_error_handler(base::Bind(
      &MojoDemuxerSourceBuffer::OnConnectionError, weak_factory_.GetWeakPtr()));
}

MojoDemuxerSourceBuffer::~MojoDemuxerSourceBuffer() {}

std::string MojoDemuxerSourceBuffer::GetDisplayName() const {
  return "MojoDemuxerSourceBuffer";
}

void MojoDemuxerSourceBuffer::Initialize(DemuxerHost* host,
                                         const PipelineStatusCB& status_cb,
                                         bool enable_text_tracks) {
  CHECK(media_task_runner_->BelongsToCurrentThread());

  MojoDemuxer::Initialize(host, status_cb, enable_text_tracks);

  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&MojoDemuxerSourceBuffer::InitializeInternal,
                            weak_factory_.GetWeakPtr()));
}

void MojoDemuxerSourceBuffer::InitializeInternal() {
  CHECK(main_task_runner_->BelongsToCurrentThread());

  mojom::SourceBufferClientAssociatedPtrInfo client_ptr_info;
  client_binding_.Bind(mojo::MakeRequest(&client_ptr_info));

  mojo::ScopedDataPipeConsumerHandle remote_consumer_handle;

  // TODO(j.isorce): fix TODO in media/mojo/common/mojo_data_buffer_converter.cc
  // CreateDataPipe first and reflect the fix here. For now the following value
  // should be enough for audio + video.
  mojo_data_buffer_writer_ = MojoDataBufferWriter::Create(
      /* capacity_num_bytes */ 3 * 1024 * 1024, &remote_consumer_handle);
  remote_source_buffer_->Initialize(std::move(client_ptr_info), GetRemoteId(),
                                    std::move(remote_consumer_handle));
}

SourceBuffer::Status MojoDemuxerSourceBuffer::AddId(const std::string& id,
                                                    const std::string& type,
                                                    const std::string& codecs) {
  CHECK(main_task_runner_->BelongsToCurrentThread());

  SourceBuffer::Status status = SourceBuffer::Status::kOk;
  remote_source_buffer_->AddId(id, type, codecs, &status);

  return status;
}

bool MojoDemuxerSourceBuffer::AppendData(const std::string& id,
                                         const uint8_t* data,
                                         size_t length,
                                         base::TimeDelta append_window_start,
                                         base::TimeDelta append_window_end,
                                         base::TimeDelta* timestamp_offset) {
  CHECK(main_task_runner_->BelongsToCurrentThread());

  scoped_refptr<media::DataBuffer> buffer =
      media::DataBuffer::CopyFrom(data, length);
  mojom::DataBufferPtr mojo_buffer =
      mojo_data_buffer_writer_->WriteDataBuffer(buffer);

  bool success = false;
  remote_source_buffer_->AppendData(id, std::move(mojo_buffer),
                                    append_window_start, append_window_end,
                                    &success, timestamp_offset);

  return success;
}

void MojoDemuxerSourceBuffer::SetTracksWatcher(
    const std::string& id,
    const MediaTracksUpdatedCB& tracks_updated_cb) {
  CHECK(main_task_runner_->BelongsToCurrentThread());

  tracks_updated_callbacks_map_[id] = tracks_updated_cb;

  remote_source_buffer_->SetTracksWatcher(id);
}

void MojoDemuxerSourceBuffer::OnTracksWatcher(
    const std::string& id,
    mojom::MediaTracksPtr mojo_tracks,
    OnTracksWatcherCallback callback) {
  CHECK(main_task_runner_->BelongsToCurrentThread());
  CHECK(!tracks_updated_callbacks_map_[id].is_null());

  if (!mojo_tracks) {
    media_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&MojoDemuxer::OnDemuxerError, weak_factory_.GetWeakPtr(),
                   PIPELINE_ERROR_INVALID_STATE));
    tracks_updated_callbacks_map_[id].Run(base::MakeUnique<MediaTracks>());
    std::move(callback).Run();
    return;
  }

  std::unique_ptr<MediaTracks> tracks(
      mojo_tracks.To<std::unique_ptr<MediaTracks>>());

  CHECK(tracks);
  tracks_updated_callbacks_map_[id].Run(std::move(tracks));
  std::move(callback).Run();
}

void MojoDemuxerSourceBuffer::SetParseWarningCallback(
    const std::string& id,
    const SourceBufferParseWarningCB& parse_warning_cb) {
  CHECK(main_task_runner_->BelongsToCurrentThread());

  tracks_parse_warning_callbacks_map_[id] = parse_warning_cb;

  remote_source_buffer_->SetParseWarningCallback(id);
}

void MojoDemuxerSourceBuffer::OnParseWarning(
    const std::string& id,
    SourceBufferParseWarning parse_warning) {
  CHECK(main_task_runner_->BelongsToCurrentThread());
  CHECK(!tracks_parse_warning_callbacks_map_[id].is_null());

  tracks_parse_warning_callbacks_map_[id].Run(parse_warning);
}

void MojoDemuxerSourceBuffer::RemoveId(const std::string& id) {
  CHECK(main_task_runner_->BelongsToCurrentThread());

  remote_source_buffer_->RemoveId(id);
}

Ranges<base::TimeDelta> MojoDemuxerSourceBuffer::GetBufferedRanges(
    const std::string& id) const {
  CHECK(main_task_runner_->BelongsToCurrentThread());

  mojom::RangesTimeDeltaPtr mojo_ranges;
  remote_source_buffer_->GetBufferedRanges(id, &mojo_ranges);

  if (!mojo_ranges) {
    media_task_runner_->PostTask(
        FROM_HERE, base::Bind(&MojoDemuxer::OnDemuxerError,
                              const_cast<MojoDemuxerSourceBuffer*>(this)
                                  ->weak_factory_.GetWeakPtr(),
                              PIPELINE_ERROR_INVALID_STATE));
    return Ranges<base::TimeDelta>();
  }

  Ranges<base::TimeDelta> ranges = mojo_ranges.To<Ranges<base::TimeDelta>>();
  return ranges;
}

base::TimeDelta MojoDemuxerSourceBuffer::GetHighestPresentationTimestamp(
    const std::string& id) const {
  CHECK(main_task_runner_->BelongsToCurrentThread());

  base::TimeDelta timestamp;
  remote_source_buffer_->GetHighestPresentationTimestamp(id, &timestamp);
  return timestamp;
}

void MojoDemuxerSourceBuffer::ResetParserState(
    const std::string& id,
    base::TimeDelta append_window_start,
    base::TimeDelta append_window_end,
    base::TimeDelta* timestamp_offset) {
  CHECK(main_task_runner_->BelongsToCurrentThread());
  DCHECK(timestamp_offset);
  remote_source_buffer_->ResetParserState(id, append_window_start,
                                          append_window_end, timestamp_offset);
}

void MojoDemuxerSourceBuffer::Remove(const std::string& id,
                                     base::TimeDelta start,
                                     base::TimeDelta end) {
  CHECK(main_task_runner_->BelongsToCurrentThread());
  remote_source_buffer_->Remove(id, start, end);
}

bool MojoDemuxerSourceBuffer::EvictCodedFrames(const std::string& id,
                                               base::TimeDelta currentMediaTime,
                                               size_t newDataSize) {
  CHECK(main_task_runner_->BelongsToCurrentThread());
  bool success = false;
  remote_source_buffer_->EvictCodedFrames(id, currentMediaTime, newDataSize,
                                          &success);
  return success;
}

void MojoDemuxerSourceBuffer::OnMemoryPressure(
    base::TimeDelta currentMediaTime,
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level,
    bool force_instant_gc) {
  CHECK(media_task_runner_->BelongsToCurrentThread());

  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&MojoDemuxerSourceBuffer::OnMemoryPressureInternal,
                            weak_factory_.GetWeakPtr(), currentMediaTime,
                            memory_pressure_level, force_instant_gc));
}

void MojoDemuxerSourceBuffer::OnMemoryPressureInternal(
    base::TimeDelta currentMediaTime,
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level,
    bool force_instant_gc) {
  CHECK(main_task_runner_->BelongsToCurrentThread());
  remote_source_buffer_->OnMemoryPressure(
      currentMediaTime, memory_pressure_level, force_instant_gc);
}

double MojoDemuxerSourceBuffer::GetDuration() {
  return duration_;
}

double MojoDemuxerSourceBuffer::GetDuration_Locked() {
  return duration_;
}

void MojoDemuxerSourceBuffer::SetDuration(double duration) {
  CHECK(main_task_runner_->BelongsToCurrentThread());
  CHECK(mojo_data_buffer_writer_);

  duration_ = duration;
  remote_source_buffer_->SetDuration(duration);
}

bool MojoDemuxerSourceBuffer::IsParsingMediaSegment(const std::string& id) {
  CHECK(main_task_runner_->BelongsToCurrentThread());
  bool is_parsing = false;
  remote_source_buffer_->IsParsingMediaSegment(id, &is_parsing);
  return is_parsing;
}

void MojoDemuxerSourceBuffer::SetSequenceMode(const std::string& id,
                                              bool sequence_mode) {
  CHECK(main_task_runner_->BelongsToCurrentThread());
  remote_source_buffer_->SetSequenceMode(id, sequence_mode);
}

void MojoDemuxerSourceBuffer::SetGroupStartTimestampIfInSequenceMode(
    const std::string& id,
    base::TimeDelta timestamp_offset) {
  CHECK(main_task_runner_->BelongsToCurrentThread());
  remote_source_buffer_->SetGroupStartTimestampIfInSequenceMode(
      id, timestamp_offset);
}

void MojoDemuxerSourceBuffer::MarkEndOfStream(PipelineStatus status) {
  CHECK(main_task_runner_->BelongsToCurrentThread());
  remote_source_buffer_->MarkEndOfStream(status);
}

void MojoDemuxerSourceBuffer::UnmarkEndOfStream() {
  CHECK(main_task_runner_->BelongsToCurrentThread());
  remote_source_buffer_->UnmarkEndOfStream();
}

void MojoDemuxerSourceBuffer::Shutdown() {
  CHECK(main_task_runner_->BelongsToCurrentThread());
  remote_source_buffer_->Shutdown();
}

void MojoDemuxerSourceBuffer::SetMemoryLimitsForTest(DemuxerStream::Type type,
                                                     size_t memory_limit) {}

Ranges<base::TimeDelta> MojoDemuxerSourceBuffer::GetBufferedRanges() const {
  return Ranges<base::TimeDelta>();
}

void MojoDemuxerSourceBuffer::OnConnectionError() {
  CHECK(main_task_runner_->BelongsToCurrentThread());

  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&MojoDemuxer::OnDemuxerError,
                            weak_factory_.GetWeakPtr(), PIPELINE_ERROR_READ));
}

void MojoDemuxerSourceBuffer::OnMediaTracksDoNothing(
    std::unique_ptr<MediaTracks> tracks) {
  // Nothing to do since it is done another way around for SourceBuffer, see
  // TrackWatcher.
}

}  // namespace media
