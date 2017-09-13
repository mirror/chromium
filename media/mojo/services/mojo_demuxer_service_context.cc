// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_demuxer_service_context.h"

#include "media/base/demuxer.h"
#include "media/mojo/services/mojo_demuxer_service.h"

namespace media {

MojoDemuxerServiceContext::MojoDemuxerServiceContext()
    : weak_ptr_factory_(this) {}

MojoDemuxerServiceContext::~MojoDemuxerServiceContext() {}

base::WeakPtr<MojoDemuxerServiceContext>
MojoDemuxerServiceContext::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void MojoDemuxerServiceContext::RegisterDemuxer(
    int demuxer_id,
    MojoDemuxerService* demuxer_service) {
  DCHECK(!demuxer_services_.count(demuxer_id));
  DCHECK(demuxer_service);
  demuxer_services_[demuxer_id] = demuxer_service;
}

void MojoDemuxerServiceContext::UnregisterDemuxer(int demuxer_id) {
  DCHECK(demuxer_services_.count(demuxer_id));
  demuxer_services_.erase(demuxer_id);
}

Demuxer* MojoDemuxerServiceContext::GetDemuxer(int demuxer_id) {
  auto demuxer_service = demuxer_services_.find(demuxer_id);
  if (demuxer_service == demuxer_services_.end()) {
    LOG(ERROR) << "Demuxer service not found: " << demuxer_id;
    return nullptr;
  }

  return demuxer_service->second->GetDemuxer();
}

media::Demuxer* MojoDemuxerServiceContext::GetDemuxerSourceBuffer(
    int demuxer_id,
    media::SourceBuffer** source_buffer_out) {
  auto demuxer_service = demuxer_services_.find(demuxer_id);
  if (demuxer_service == demuxer_services_.end()) {
    LOG(ERROR) << "Demuxer service not found: " << demuxer_id;
    return nullptr;
  }

  return demuxer_service->second->GetDemuxerSourceBuffer(source_buffer_out);
}

}  // namespace media
