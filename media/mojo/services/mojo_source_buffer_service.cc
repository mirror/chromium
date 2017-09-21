// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_source_buffer_service.h"

#include <map>

#include "media/base/data_buffer.h"
#include "media/base/media_tracks.h"
#include "media/base/source_buffer.h"
#include "media/mojo/common/media_type_converters.h"
#include "media/mojo/common/mojo_data_buffer_converter.h"

namespace media {

MojoSourceBufferService::MojoSourceBufferService(
    base::WeakPtr<MojoDemuxerServiceContext> context)
    : task_runner_(base::ThreadTaskRunnerHandle::Get()),
      context_(context),
      source_buffer_(nullptr),
      weak_factory_(this) {
  DCHECK(context_);
}

MojoSourceBufferService::~MojoSourceBufferService() {}

void MojoSourceBufferService::Initialize(
    mojom::SourceBufferClientAssociatedPtrInfo client,
    int32_t demuxer_id,
    mojo::ScopedDataPipeConsumerHandle consumer_handle) {
  CHECK(task_runner_->BelongsToCurrentThread());

  client_.Bind(std::move(client));

  context_->GetDemuxerSourceBuffer(demuxer_id, &source_buffer_);
  CHECK(source_buffer_);

  mojo_data_buffer_reader_.reset(
      new MojoDataBufferReader(std::move(consumer_handle)));
}

void MojoSourceBufferService::AddId(const std::string& id,
                                    const std::string& type,
                                    const std::string& codecs,
                                    AddIdCallback callback) {
  CHECK(task_runner_->BelongsToCurrentThread());
  media::SourceBuffer::Status status = source_buffer_->AddId(id, type, codecs);
  std::move(callback).Run(status);
}

void MojoSourceBufferService::AppendData(const std::string& id,
                                         mojom::DataBufferPtr mojo_data_buffer,
                                         base::TimeDelta append_window_start,
                                         base::TimeDelta append_window_end,
                                         AppendDataCallback callback) {
  CHECK(task_runner_->BelongsToCurrentThread());

  mojo_data_buffer_reader_->ReadDataBuffer(
      std::move(mojo_data_buffer),
      base::BindOnce(&MojoSourceBufferService::OnBufferRead,
                     weak_factory_.GetWeakPtr(), id, append_window_start,
                     append_window_end, base::Passed(&callback)));
}

void MojoSourceBufferService::OnBufferRead(const std::string& id,
                                           base::TimeDelta append_window_start,
                                           base::TimeDelta append_window_end,
                                           AppendDataCallback callback,
                                           scoped_refptr<DataBuffer> buffer) {
  CHECK(task_runner_->BelongsToCurrentThread());
  base::TimeDelta timestamp_offset;
  bool success = false;

  if (!buffer) {
    std::move(callback).Run(success, timestamp_offset);
    return;
  }
  success = source_buffer_->AppendData(id, buffer->data(), buffer->data_size(),
                                       append_window_start, append_window_end,
                                       &timestamp_offset);
  std::move(callback).Run(success, timestamp_offset);
}

void MojoSourceBufferService::SetTracksWatcher(const std::string& id) {
  CHECK(task_runner_->BelongsToCurrentThread());
  source_buffer_->SetTracksWatcher(
      id, base::Bind(&MojoSourceBufferService::OnTracksWatcher,
                     weak_factory_.GetWeakPtr(), id));
}

void MojoSourceBufferService::OnTracksWatcher(
    const std::string& id,
    std::unique_ptr<MediaTracks> tracks) {
  CHECK(task_runner_->BelongsToCurrentThread());
  CHECK(tracks);
  mojom::MediaTracksPtr mojo_tracks = mojom::MediaTracks::From(*tracks.get());
  CHECK(mojo_tracks);

  client_->OnTracksWatcher(id, std::move(mojo_tracks));
}

void MojoSourceBufferService::SetParseWarningCallback(const std::string& id) {
  CHECK(task_runner_->BelongsToCurrentThread());
  source_buffer_->SetParseWarningCallback(
      id, base::Bind(&MojoSourceBufferService::OnParseWarning,
                     weak_factory_.GetWeakPtr(), id));
}

void MojoSourceBufferService::OnParseWarning(
    const std::string& id,
    SourceBufferParseWarning parse_warning) {
  CHECK(task_runner_->BelongsToCurrentThread());
  client_->OnParseWarning(id, parse_warning);
}

void MojoSourceBufferService::RemoveId(const std::string& id) {
  CHECK(task_runner_->BelongsToCurrentThread());
  source_buffer_->RemoveId(id);
}

void MojoSourceBufferService::GetBufferedRanges(
    const std::string& id,
    GetBufferedRangesCallback callback) {
  CHECK(task_runner_->BelongsToCurrentThread());
  Ranges<base::TimeDelta> ranges = source_buffer_->GetBufferedRanges(id);
  mojom::RangesTimeDeltaPtr mojo_ranges = mojom::RangesTimeDelta::From(ranges);
  CHECK(mojo_ranges);
  std::move(callback).Run(std::move(mojo_ranges));
}

void MojoSourceBufferService::GetHighestPresentationTimestamp(
    const std::string& id,
    GetHighestPresentationTimestampCallback callback) {
  CHECK(task_runner_->BelongsToCurrentThread());
  base::TimeDelta timestamp =
      source_buffer_->GetHighestPresentationTimestamp(id);
  std::move(callback).Run(timestamp);
}

void MojoSourceBufferService::ResetParserState(
    const std::string& id,
    base::TimeDelta append_window_start,
    base::TimeDelta append_window_end,
    ResetParserStateCallback callback) {
  CHECK(task_runner_->BelongsToCurrentThread());
  base::TimeDelta timestamp_offset;
  source_buffer_->ResetParserState(id, append_window_start, append_window_end,
                                   &timestamp_offset);
  std::move(callback).Run(timestamp_offset);
}

void MojoSourceBufferService::Remove(const std::string& id,
                                     base::TimeDelta start,
                                     base::TimeDelta end) {
  CHECK(task_runner_->BelongsToCurrentThread());
  source_buffer_->Remove(id, start, end);
}

void MojoSourceBufferService::EvictCodedFrames(
    const std::string& id,
    base::TimeDelta currentMediaTime,
    size_t newDataSize,
    EvictCodedFramesCallback callback) {
  CHECK(task_runner_->BelongsToCurrentThread());
  bool success =
      source_buffer_->EvictCodedFrames(id, currentMediaTime, newDataSize);
  std::move(callback).Run(success);
}

void MojoSourceBufferService::OnMemoryPressure(
    base::TimeDelta currentMediaTime,
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level,
    bool force_instant_gc) {
  CHECK(task_runner_->BelongsToCurrentThread());
  source_buffer_->OnMemoryPressure(currentMediaTime, memory_pressure_level,
                                   force_instant_gc);
}

void MojoSourceBufferService::GetDuration(GetDurationCallback callback) {
  double duration = source_buffer_->GetDuration();
  std::move(callback).Run(duration);
}

void MojoSourceBufferService::GetDuration_Locked(
    GetDuration_LockedCallback callback) {
  std::move(callback).Run(source_buffer_->GetDuration_Locked());
}

void MojoSourceBufferService::SetDuration(double duration) {
  source_buffer_->SetDuration(duration);
}

void MojoSourceBufferService::IsParsingMediaSegment(
    const std::string& id,
    IsParsingMediaSegmentCallback callback) {
  bool is_parsing = source_buffer_->IsParsingMediaSegment(id);
  std::move(callback).Run(is_parsing);
}

void MojoSourceBufferService::SetSequenceMode(const std::string& id,
                                              bool sequence_mode) {
  source_buffer_->SetSequenceMode(id, sequence_mode);
}

void MojoSourceBufferService::SetGroupStartTimestampIfInSequenceMode(
    const std::string& id,
    base::TimeDelta timestamp_offset) {
  source_buffer_->SetGroupStartTimestampIfInSequenceMode(id, timestamp_offset);
}

void MojoSourceBufferService::MarkEndOfStream(PipelineStatus status) {
  source_buffer_->MarkEndOfStream(status);
}

void MojoSourceBufferService::UnmarkEndOfStream() {
  source_buffer_->UnmarkEndOfStream();
}

void MojoSourceBufferService::Shutdown() {
  source_buffer_->Shutdown();
}
/*
void MojoSourceBufferService::SetMemoryLimitsForTest(DemuxerStream::Type type,
size_t memory_limit){}

Ranges<base::TimeDelta> MojoSourceBufferService::GetBufferedRanges() const{
  return source_buffer_->GetBufferedRanges();
}*/

}  // namespace media
