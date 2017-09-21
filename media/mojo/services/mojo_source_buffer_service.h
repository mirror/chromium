// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_SOURCE_BUFFER_SERVICE_H_
#define MEDIA_MOJO_SERVICES_MOJO_SOURCE_BUFFER_SERVICE_H_

#include "media/base/source_buffer.h"
#include "media/mojo/interfaces/source_buffer.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "media/mojo/services/mojo_demuxer_service_context.h"

namespace media {

class DataBuffer;
class MediaTracks;
class MojoDataBufferReader;

// A mojom::SourceBuffer implementation backed by a media::SourceBuffer.
class MEDIA_MOJO_EXPORT MojoSourceBufferService : public mojom::SourceBuffer {
 public:
  MojoSourceBufferService(base::WeakPtr<MojoDemuxerServiceContext> context);

  ~MojoSourceBufferService() final;

  // mojom::SourceBuffer implementation.
  void Initialize(mojom::SourceBufferClientAssociatedPtrInfo client,
                  int32_t demuxer_id,
                  mojo::ScopedDataPipeConsumerHandle consumer_handle) final;
  void AddId(const std::string& id,
             const std::string& type,
             const std::string& codecs,
             AddIdCallback callback) final;
  void AppendData(const std::string& id,
                  mojom::DataBufferPtr mojo_data_buffer,
                  base::TimeDelta append_window_start,
                  base::TimeDelta append_window_end,
                  AppendDataCallback callback) final;
  void SetTracksWatcher(const std::string& id) final;
  void SetParseWarningCallback(const std::string& id) final;
  void RemoveId(const std::string& id) final;
  void GetBufferedRanges(const std::string& id,
                         GetBufferedRangesCallback callback) final;
  void GetHighestPresentationTimestamp(
      const std::string& id,
      GetHighestPresentationTimestampCallback callback) final;
  void ResetParserState(const std::string& id,
                        base::TimeDelta append_window_start,
                        base::TimeDelta append_window_end,
                        ResetParserStateCallback callback) final;
  void Remove(const std::string& id,
              base::TimeDelta start,
              base::TimeDelta end) final;
  void EvictCodedFrames(const std::string& id,
                        base::TimeDelta currentMediaTime,
                        size_t newDataSize,
                        EvictCodedFramesCallback callback) final;
  void OnMemoryPressure(
      base::TimeDelta currentMediaTime,
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level,
      bool force_instant_gc) final;
  void GetDuration(GetDurationCallback callback) final;
  void GetDuration_Locked(GetDuration_LockedCallback callback) final;
  void SetDuration(double duration) final;
  void IsParsingMediaSegment(const std::string& id,
                             IsParsingMediaSegmentCallback callback) final;
  void SetSequenceMode(const std::string& id, bool sequence_mode) final;
  void SetGroupStartTimestampIfInSequenceMode(
      const std::string& id,
      base::TimeDelta timestamp_offset) final;
  void MarkEndOfStream(PipelineStatus status) final;
  void UnmarkEndOfStream() final;
  void Shutdown() final;
  /*void SetMemoryLimitsForTest(DemuxerStream::Type type, size_t memory_limit)
  final;
  Ranges<base::TimeDelta> GetBufferedRanges() const final;*/

 private:
  void OnBufferRead(const std::string& id,
                    base::TimeDelta append_window_start,
                    base::TimeDelta append_window_end,
                    AppendDataCallback callback,
                    scoped_refptr<media::DataBuffer> buffer);
  void OnTracksWatcher(const std::string& id,
                       std::unique_ptr<MediaTracks> tracks);
  void OnParseWarning(const std::string& id,
                      SourceBufferParseWarning parse_warning);

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  mojom::SourceBufferClientAssociatedPtr client_;

  base::WeakPtr<MojoDemuxerServiceContext> context_;

  media::SourceBuffer* source_buffer_;

  std::unique_ptr<MojoDataBufferReader> mojo_data_buffer_reader_;

  base::WeakPtrFactory<MojoSourceBufferService> weak_factory_;
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_SOURCE_BUFFER_SERVICE_H_
