// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_CLIENTS_MOJO_DEMUXER_H_
#define MEDIA_MOJO_CLIENTS_MOJO_DEMUXER_H_

#include "media/base/demuxer.h"
#include "media/base/demuxer_stream.h"
#include "media/mojo/interfaces/demuxer.mojom.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

namespace media {

class DataSource;
class MojoDataSourceImpl;

class TrackDemuxerStream : public DemuxerStream {
 public:
  TrackDemuxerStream(const AudioDecoderConfig& audio_decoder_config);
  TrackDemuxerStream(const VideoDecoderConfig& video_decoder_config);
  ~TrackDemuxerStream() override;

  // DemuxerStream methods.
  void Read(const ReadCB& read_cb) override;
  DemuxerStream::Type type() const override;
  DemuxerStream::Liveness liveness() const override;
  AudioDecoderConfig audio_decoder_config() override;
  VideoDecoderConfig video_decoder_config() override;
  bool SupportsConfigChanges() override;
  VideoRotation video_rotation() override;

 private:
  DemuxerStream::Type type_;
  AudioDecoderConfig audio_decoder_config_;
  VideoDecoderConfig video_decoder_config_;
};

class MojoDemuxer : public Demuxer, public mojom::DemuxerClient {
 public:
  MojoDemuxer(
      int32_t remote_id,
      Demuxer::LoadType load_type,
      DataSource* data_source,
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      mojom::DemuxerPtr mojo_demuxer,
      const base::Closure& open_cb,
      const base::Closure& progress_cb,
      const Demuxer::EncryptedMediaInitDataCB& encrypted_media_init_data_cb,
      const Demuxer::MediaTracksUpdatedCB& media_tracks_updated_cb);
  ~MojoDemuxer() override;

  // MediaResource interface.
  MediaResource::Type GetType() const override;
  std::vector<DemuxerStream*> GetAllStreams() override;
  void SetStreamStatusChangeCB(const StreamStatusChangeCB& cb) override;

  // Demuxer interface.
  std::string GetDisplayName() const override;
  void Initialize(DemuxerHost* host,
                  const PipelineStatusCB& status_cb,
                  bool enable_text_tracks) override;
  void StartWaitingForSeek(base::TimeDelta seek_time) override;
  void CancelPendingSeek(base::TimeDelta seek_time) override;
  void Seek(base::TimeDelta time, const PipelineStatusCB& status_cb) override;
  void Stop() override;
  void AbortPendingReads() override;
  base::TimeDelta GetStartTime() const override;
  base::Time GetTimelineOffset() const override;
  int64_t GetMemoryUsage() const override;
  void OnEnabledAudioTracksChanged(const std::vector<MediaTrack::Id>& track_ids,
                                   base::TimeDelta current_time) override;
  void OnSelectedVideoTrackChanged(
      base::Optional<MediaTrack::Id> selected_track_id,
      base::TimeDelta current_time) override;

  // mojom::DemuxerClient implementation.
  void OnEncryptedMediaInitData(EmeInitDataType init_data_type,
                                const std::vector<uint8_t>& init_data) final;
  void OnMediaTracksUpdated(mojom::MediaTracksPtr mojo_tracks) final;
  void OnBufferedTimeRangesChanged(mojom::RangesTimeDeltaPtr mojo_ranges) final;
  void OnSetDuration(base::TimeDelta duration) final;
  void OnOpened() final;
  void OnProgress() final;
  void OnDemuxerError(PipelineStatus error) final;

 protected:
  scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;

 private:
  void OnDataSourceConnectionError();
  void OnInitialized(PipelineStatus status);
  void OnSeek(const PipelineStatusCB& status_cb, PipelineStatus status);
  void OnConnectionError();

  Demuxer::LoadType load_type_;
  std::vector<TrackDemuxerStream> track_demuxer_streams_;

  std::unique_ptr<MojoDataSourceImpl> mojo_data_source_;
  DataSource* data_source_;

  base::Closure open_cb_;
  base::Closure progress_cb_;
  Demuxer::EncryptedMediaInitDataCB encrypted_media_init_data_cb_;
  Demuxer::MediaTracksUpdatedCB media_tracks_updated_cb_;
  PipelineStatusCB status_cb_;

  DemuxerHost* host_;

  // This class is constructed on one thread and used exclusively on another
  // thread. This member is used to safely pass the DemuxerPtr from one thread
  // to another. It is set in the constructor and is consumed in Initialize().
  mojom::DemuxerPtrInfo remote_demuxer_info_;

  mojom::DemuxerPtr remote_demuxer_;

  // Binding for DemuxerClient, bound to the |media_task_runner_|.
  mojo::AssociatedBinding<DemuxerClient> client_binding_;

  base::WeakPtrFactory<MojoDemuxer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MojoDemuxer);
};

}  // namespace media

#endif  // MEDIA_MOJO_CLIENTS_MOJO_DEMUXER_H_
