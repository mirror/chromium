// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FILTERS_DEFAULT_DEMUXER_FACTORY_H_
#define MEDIA_FILTERS_DEFAULT_DEMUXER_FACTORY_H_

#include "media/base/demuxer_factory.h"

namespace media {

class MediaLog;

// The default factory class for creating Demuxers.
class MEDIA_EXPORT DefaultDemuxerFactory : public DemuxerFactory {
 public:
  DefaultDemuxerFactory(MediaLog* media_log);
  ~DefaultDemuxerFactory() final;

  std::unique_ptr<Demuxer> CreateDemuxer(
      const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      DataSource* data_source,
      const Demuxer::EncryptedMediaInitDataCB& encrypted_media_init_data_cb,
      const Demuxer::MediaTracksUpdatedCB& media_tracks_updated_cb) final;

  std::unique_ptr<Demuxer> CreateDemuxerSourceBuffer(
      const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      const base::Closure& open_cb,
      const base::Closure& progress_cb,
      const Demuxer::EncryptedMediaInitDataCB& encrypted_media_init_data_cb,
      media::SourceBuffer** source_buffer_out) final;

 private:
  MediaLog* media_log_;

  DISALLOW_COPY_AND_ASSIGN(DefaultDemuxerFactory);
};

}  // namespace media

#endif  // MEDIA_FILTERS_DEFAULT_DEMUXER_FACTORY_H_
