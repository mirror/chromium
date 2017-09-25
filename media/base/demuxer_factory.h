// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_DEMUXER_FACTORY_H_
#define MEDIA_BASE_DEMUXER_FACTORY_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "media/base/demuxer.h"
#include "media/base/media_export.h"
#include "media/base/source_buffer.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

// A factory class for creating media::Demuxer to be used by media pipeline.
class MEDIA_EXPORT DemuxerFactory {
 public:
  DemuxerFactory();
  virtual ~DemuxerFactory();

  virtual std::unique_ptr<Demuxer> CreateDemuxer(
      const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      DataSource* data_source,
      const Demuxer::EncryptedMediaInitDataCB& encrypted_media_init_data_cb,
      const Demuxer::MediaTracksUpdatedCB& media_tracks_updated_cb) = 0;

  virtual std::unique_ptr<Demuxer> CreateDemuxerSourceBuffer(
      const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      const base::Closure& open_cb,
      const base::Closure& progress_cb,
      const Demuxer::EncryptedMediaInitDataCB& encrypted_media_init_data_cb,
      media::SourceBuffer** source_buffer_out) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(DemuxerFactory);
};

}  // namespace media

#endif  // MEDIA_BASE_DEMUXER_FACTORY_H_
