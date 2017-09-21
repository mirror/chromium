// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_CLIENTS_MOJO_DEMUXER_SOURCE_BUFFER_H_
#define MEDIA_MOJO_CLIENTS_MOJO_DEMUXER_SOURCE_BUFFER_H_

#include "media/base/source_buffer.h"
#include "media/mojo/clients/mojo_demuxer.h"
#include "media/mojo/interfaces/source_buffer.mojom.h"

namespace media {

class MojoDataBufferWriter;

class MojoDemuxerSourceBuffer : public MojoDemuxer,
                                public SourceBuffer,
                                public mojom::SourceBufferClient {
 public:
  explicit MojoDemuxerSourceBuffer(
      int32_t id,
      const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      mojom::DemuxerPtr mojo_demuxer,
      mojom::SourceBufferPtr mojo_source_buffer,
      const base::Closure& open_cb,
      const base::Closure& progress_cb,
      const Demuxer::EncryptedMediaInitDataCB& encrypted_media_init_data_cb);
  ~MojoDemuxerSourceBuffer() override;

  // Demuxer interface.
  std::string GetDisplayName() const override;
  void Initialize(DemuxerHost* host,
                  const PipelineStatusCB& status_cb,
                  bool enable_text_tracks) override;

  // SourceBuffer interface.
  SourceBuffer::Status AddId(const std::string& id,
                             const std::string& type,
                             const std::string& codecs) final;
  void SetTracksWatcher(const std::string& id,
                        const MediaTracksUpdatedCB& tracks_updated_cb) final;
  void SetParseWarningCallback(
      const std::string& id,
      const SourceBufferParseWarningCB& parse_warning_cb) final;
  void RemoveId(const std::string& id) final;
  Ranges<base::TimeDelta> GetBufferedRanges(const std::string& id) const final;
  base::TimeDelta GetHighestPresentationTimestamp(
      const std::string& id) const final;
  bool AppendData(const std::string& id,
                  const uint8_t* data,
                  size_t length,
                  base::TimeDelta append_window_start,
                  base::TimeDelta append_window_end,
                  base::TimeDelta* timestamp_offset) final;
  void ResetParserState(const std::string& id,
                        base::TimeDelta append_window_start,
                        base::TimeDelta append_window_end,
                        base::TimeDelta* timestamp_offset) final;
  void Remove(const std::string& id,
              base::TimeDelta start,
              base::TimeDelta end) final;
  bool EvictCodedFrames(const std::string& id,
                        base::TimeDelta currentMediaTime,
                        size_t newDataSize) final;
  void OnMemoryPressure(
      base::TimeDelta currentMediaTime,
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level,
      bool force_instant_gc) final;
  double GetDuration() final;
  double GetDuration_Locked() final;
  void SetDuration(double duration) final;
  bool IsParsingMediaSegment(const std::string& id) final;
  void SetSequenceMode(const std::string& id, bool sequence_mode) final;
  void SetGroupStartTimestampIfInSequenceMode(
      const std::string& id,
      base::TimeDelta timestamp_offset) final;
  void MarkEndOfStream(PipelineStatus status) final;
  void UnmarkEndOfStream() final;
  void Shutdown() final;
  void SetMemoryLimitsForTest(DemuxerStream::Type type,
                              size_t memory_limit) final;
  Ranges<base::TimeDelta> GetBufferedRanges() const final;

  // mojom::SourceBufferClient implementation.
  void OnTracksWatcher(const std::string& id,
                       mojom::MediaTracksPtr mojo_tracks,
                       OnTracksWatcherCallback callback) final;
  void OnParseWarning(const std::string& id,
                      SourceBufferParseWarning parse_warning) final;

 private:
  void InitializeInternal();
  void OnMemoryPressureInternal(
      base::TimeDelta currentMediaTime,
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level,
      bool force_instant_gc);
  void OnConnectionError();
  void OnMediaTracksDoNothing(std::unique_ptr<MediaTracks> tracks);

  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  mojom::SourceBufferPtr remote_source_buffer_;

  // Binding for SourceBufferClient, bound to the |main_task_runner_|.
  mojo::AssociatedBinding<SourceBufferClient> client_binding_;

  double duration_;

  std::unique_ptr<MojoDataBufferWriter> mojo_data_buffer_writer_;

  std::map<std::string, MediaTracksUpdatedCB> tracks_updated_callbacks_map_;

  std::map<std::string, SourceBufferParseWarningCB>
      tracks_parse_warning_callbacks_map_;

  base::WeakPtrFactory<MojoDemuxerSourceBuffer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MojoDemuxerSourceBuffer);
};

}  // namespace media

#endif  // MEDIA_MOJO_CLIENTS_MOJO_DEMUXER_SOURCE_BUFFER_H_
