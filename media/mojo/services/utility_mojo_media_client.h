// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_UTILITY_MOJO_MEDIA_CLIENT_H_
#define MEDIA_MOJO_SERVICES_UTILITY_MOJO_MEDIA_CLIENT_H_

#include <memory>

#include "media/mojo/services/mojo_media_client.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

class VideoDecoder;

// Default MojoMediaClient for MediaService.
class UtilityMojoMediaClient : public MojoMediaClient {
 public:
  UtilityMojoMediaClient();
  ~UtilityMojoMediaClient() final;

  // MojoMediaClient implementation.
  void Initialize(
      service_manager::Connector* connector,
      service_manager::ServiceContextRefFactory* context_ref_factory) final;
  std::unique_ptr<VideoDecoder> CreateVideoDecoder(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      MediaLog* media_log,
      mojom::CommandBufferIdPtr command_buffer_id,
      OutputWithReleaseMailboxCB output_cb) final;

  DISALLOW_COPY_AND_ASSIGN(UtilityMojoMediaClient);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_UTILITY_MOJO_MEDIA_CLIENT_H_
