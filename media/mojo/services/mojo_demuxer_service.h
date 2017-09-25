// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_DEMUXER_SERVICE_H_
#define MEDIA_MOJO_SERVICES_MOJO_DEMUXER_SERVICE_H_

#include "media/base/demuxer.h"
#include "media/mojo/interfaces/data_source.mojom.h"
#include "media/mojo/interfaces/demuxer.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "media/mojo/services/mojo_demuxer_service_context.h"

namespace media {

class DataSource;
class DemuxerFactory;
class MediaTracks;
class SourceBuffer;

// A mojom::Demuxer implementation backed by a
// media::Demuxer.
class MEDIA_MOJO_EXPORT MojoDemuxerService : public DemuxerHost,
                                             public mojom::Demuxer {
 public:
  MojoDemuxerService(base::WeakPtr<MojoDemuxerServiceContext> context,
                     DemuxerFactory* demuxer_factory);
  ~MojoDemuxerService() final;

  media::Demuxer* GetDemuxerSourceBuffer(
      media::SourceBuffer** source_buffer_out);
  media::Demuxer* GetDemuxer();

  void OnEncryptedMediaInitData(EmeInitDataType init_data_type,
                                const std::vector<uint8_t>& init_data);
  void OnMediaTracksUpdated(std::unique_ptr<MediaTracks> tracks);

  // DemuxHost implementation.
  void OnBufferedTimeRangesChanged(const Ranges<base::TimeDelta>& ranges) final;
  void SetDuration(base::TimeDelta duration) final;
  void OnDemuxerError(PipelineStatus error) final;
  void AddTextStream(DemuxerStream* text_stream,
                     const TextTrackConfig& config) final;
  void RemoveTextStream(DemuxerStream* text_stream) final;

  // mojom::Demuxer implementation.
  void Initialize(mojom::DemuxerClientAssociatedPtrInfo client,
                  int32_t demuxer_id,
                  media::Demuxer::LoadType load_type,
                  mojom::DataSourcePtr mojo_data_source,
                  InitializeCallback callback) final;
  void StartWaitingForSeek(base::TimeDelta seek_time) final;
  void CancelPendingSeek(base::TimeDelta seek_time) final;
  void Seek(base::TimeDelta time, SeekCallback callback) final;
  void Stop() final;
  void AbortPendingReads() final;
  void GetStartTime(GetStartTimeCallback callback) final;
  void GetTimelineOffset(GetTimelineOffsetCallback callback) final;
  void GetMemoryUsage(GetMemoryUsageCallback callback) final;
  void OnEnabledAudioTracksChanged(const std::vector<MediaTrack::Id>& track_ids,
                                   base::TimeDelta current_time) final;
  void OnSelectedVideoTrackChanged(
      const base::Optional<MediaTrack::Id>& selected_track_id,
      base::TimeDelta current_time) final;
  void GetAllStreams(GetAllStreamsCallback callback) final;

 private:
  void OnDataSourceReady(InitializeCallback callback);
  void OnDemuxerInitialized(InitializeCallback callback, PipelineStatus status);
  void OnSeek(SeekCallback callback, PipelineStatus status);
  void OnOpened();
  void OnProgress();

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  base::WeakPtr<MojoDemuxerServiceContext> context_;
  DemuxerFactory* demuxer_factory_;

  mojom::DemuxerClientAssociatedPtr client_;

  std::unique_ptr<media::DataSource> data_source_;

  int32_t demuxer_id_;
  std::unique_ptr<media::Demuxer> demuxer_;
  media::SourceBuffer* source_buffer_;

  base::WeakPtrFactory<MojoDemuxerService> weak_factory_;
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_DEMUXER_SERVICE_H_
