// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/clients/mojo_demuxer_factory.h"

#include "media/mojo/clients/mojo_demuxer_source_buffer.h"
#include "media/mojo/interfaces/interface_factory.mojom.h"
#include "services/service_manager/public/cpp/connect.h"

namespace media {

int MojoDemuxerFactory::next_demuxer_id_ = MediaResource::kInvalidRemoteId + 1;

MojoDemuxerFactory::MojoDemuxerFactory(
    media::mojom::InterfaceFactory* interface_factory)
    : interface_factory_(interface_factory) {
  DCHECK(interface_factory_);
}

MojoDemuxerFactory::~MojoDemuxerFactory() {}

std::unique_ptr<Demuxer> MojoDemuxerFactory::CreateDemuxer(
    const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    DataSource* data_source,
    const Demuxer::EncryptedMediaInitDataCB& encrypted_media_init_data_cb,
    const Demuxer::MediaTracksUpdatedCB& media_tracks_updated_cb) {
  CHECK(main_task_runner->BelongsToCurrentThread());

  mojom::DemuxerPtr demuxer_ptr;
  interface_factory_->CreateDemuxer(mojo::MakeRequest(&demuxer_ptr));

  return base::MakeUnique<MojoDemuxer>(
      next_demuxer_id_++, Demuxer::LoadType::LoadTypeURL, data_source,
      media_task_runner, std::move(demuxer_ptr), base::Bind(&base::DoNothing),
      base::Bind(&base::DoNothing), encrypted_media_init_data_cb,
      media_tracks_updated_cb);
}

std::unique_ptr<Demuxer> MojoDemuxerFactory::CreateDemuxerSourceBuffer(
    const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    const base::Closure& open_cb,
    const base::Closure& progress_cb,
    const Demuxer::EncryptedMediaInitDataCB& encrypted_media_init_data_cb,
    media::SourceBuffer** source_buffer_out) {
  DCHECK(source_buffer_out);
  mojom::DemuxerPtr demuxer_ptr;
  interface_factory_->CreateDemuxer(mojo::MakeRequest(&demuxer_ptr));

  mojom::SourceBufferPtr source_buffer_ptr;
  interface_factory_->CreateSourceBuffer(mojo::MakeRequest(&source_buffer_ptr));

  MojoDemuxerSourceBuffer* mojo_demuxer_source_buffer =
      new MojoDemuxerSourceBuffer(next_demuxer_id_++, main_task_runner,
                                  media_task_runner, std::move(demuxer_ptr),
                                  std::move(source_buffer_ptr), open_cb,
                                  progress_cb, encrypted_media_init_data_cb);

  std::unique_ptr<Demuxer> demuxer(mojo_demuxer_source_buffer);

  *source_buffer_out = mojo_demuxer_source_buffer;

  return demuxer;
}

}  // namespace media
