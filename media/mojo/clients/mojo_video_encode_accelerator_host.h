// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_CLIENTS_MOJO_VIDEO_ENCODE_ACCELERATOR_HOST_H_
#define MEDIA_MOJO_CLIENTS_MOJO_VIDEO_ENCODE_ACCELERATOR_HOST_H_

#include <stdint.h>

#include <vector>

#include "base/sequence_checker.h"
#include "gpu/ipc/client/command_buffer_proxy_impl.h"
#include "media/mojo/interfaces/video_encode_accelerator.mojom.h"
#include "media/video/video_encode_accelerator.h"

namespace gfx {
class Size;
}  // namespace gfx

namespace gpu {
class CommandBufferProxyImpl;
class GpuChannelHost;
}  // namespace gpu

namespace media {
class VideoFrame;
}  // namespace media

namespace media {

// This class is a renderer-side host bridge from VideoEncodeAccelerator to a
// remote media::mojom::VideoEncodeAccelerator passed on ctor and held as
// |vea_|. It also owns an internal |vea_client_| acting as the remote's mojo
// VEA client, trampolining methods to the Client passed on Initialize().
class MojoVideoEncodeAcceleratorHost : public VideoEncodeAccelerator {
 public:
  MojoVideoEncodeAcceleratorHost(mojom::VideoEncodeAcceleratorPtr vea,
                                 gpu::CommandBufferProxyImpl* impl);

  // VideoEncodeAccelerator implementation.
  SupportedProfiles GetSupportedProfiles() override;
  bool Initialize(VideoPixelFormat input_format,
                  const gfx::Size& input_visible_size,
                  VideoCodecProfile output_profile,
                  uint32_t initial_bitrate,
                  Client* client) override;
  void Encode(const scoped_refptr<VideoFrame>& frame,
              bool force_keyframe) override;
  void UseOutputBitstreamBuffer(const BitstreamBuffer& buffer) override;
  void RequestEncodingParametersChange(uint32_t bitrate,
                                       uint32_t framerate_num) override;
  void Destroy() override;

 private:
  // Only Destroy() should be deleting |this|.
  ~MojoVideoEncodeAcceleratorHost() override;

  mojom::VideoEncodeAcceleratorPtr vea_;
  mojom::VideoEncodeAcceleratorClientPtr vea_client_;

  // Kept only for querying
  scoped_refptr<gpu::GpuChannelHost> channel_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(MojoVideoEncodeAcceleratorHost);
};

}  // namespace media

#endif  // MEDIA_MOJO_CLIENTS_MOJO_VIDEO_ENCODE_ACCELERATOR_HOST_H_
