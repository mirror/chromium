// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_DEMUXER_SERVICE_CONTEXT_H_
#define MEDIA_MOJO_SERVICES_MOJO_DEMUXER_SERVICE_CONTEXT_H_

#include <map>

#include "base/memory/weak_ptr.h"
#include "media/base/pipeline.h"
#include "media/mojo/services/media_mojo_export.h"

namespace media {

class Demuxer;
class MojoDemuxerService;
class SourceBuffer;

// A class that creates, owns and manages all MojoDemuxerService instances.
class MEDIA_MOJO_EXPORT MojoDemuxerServiceContext {
 public:
  MojoDemuxerServiceContext();
  ~MojoDemuxerServiceContext();

  base::WeakPtr<MojoDemuxerServiceContext> GetWeakPtr();

  // Registers The |demuxer_service| with |demuxer_id|.
  void RegisterDemuxer(int demuxer_id, MojoDemuxerService* demuxer_service);

  // Unregisters the Demuxer. Must be called before the Demuxer is destroyed.
  void UnregisterDemuxer(int demuxer_id);

  // Returns the Demuxer associated with |demuxer_id|.
  Demuxer* GetDemuxer(int demuxer_id);

  media::Demuxer* GetDemuxerSourceBuffer(
      int demuxer_id,
      media::SourceBuffer** source_buffer_out);

 private:
  // A map between Demuxer ID and MojoDemuxerService.
  std::map<int, MojoDemuxerService*> demuxer_services_;

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<MojoDemuxerServiceContext> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MojoDemuxerServiceContext);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_DEMUXER_SERVICE_CONTEXT_H_
