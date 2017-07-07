// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_VIDEO_ENCODE_ACCELERATOR_H_
#define MEDIA_MOJO_SERVICES_MOJO_VIDEO_ENCODE_ACCELERATOR_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "media/mojo/interfaces/video_encode_accelerator.mojom.h"
#include "media/video/video_encode_accelerator.h"

namespace media {

// This class implements the interface mojom::VideoEncodeAccelerator.
class MojoVideoEncodeAccelerator : public mojom::VideoEncodeAccelerator,
                                   public VideoEncodeAccelerator::Client {
 public:
  static void Create(media::mojom::VideoEncodeAcceleratorRequest request);

  MojoVideoEncodeAccelerator();
  ~MojoVideoEncodeAccelerator() override;

  // mojom::VideoEncodeAccelerator impl.
  void Initialize(media::VideoPixelFormat input_format,
                  const gfx::Size& input_visible_size,
                  media::VideoCodecProfile output_profile,
                  uint32_t initial_bitrate,
                  mojom::VideoEncodeAcceleratorClientPtr client) override;
  void Encode(const scoped_refptr<VideoFrame>& frame,
              bool force_keyframe,
              const EncodeCallback& callback) override;
  void UseOutputBitstreamBuffer(int32_t bitstream_buffer_id,
                                mojo::ScopedSharedBufferHandle buffer) override;
  void RequestEncodingParametersChange(uint32_t bitrate,
                                       uint32_t framerate) override;

  // VideoEncodeAccelerator::Client implementation.
  void RequireBitstreamBuffers(unsigned int input_count,
                               const gfx::Size& input_coded_size,
                               size_t output_buffer_size) override;
  void BitstreamBufferReady(int32_t bitstream_buffer_id,
                            size_t payload_size,
                            bool key_frame,
                            base::TimeDelta timestamp) override;
  void NotifyError(::media::VideoEncodeAccelerator::Error error) override;

 private:
  // Owned pointer to the underlying VideoEncodeAccelerator.
  std::unique_ptr<::media::VideoEncodeAccelerator> encoder_;
  mojom::VideoEncodeAcceleratorClientPtr vea_client_;

  // Cache of parameters for sanity verification.
  size_t output_buffer_size_;

  // Note that this class is already thread hostile when bound.
  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<MojoVideoEncodeAccelerator> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MojoVideoEncodeAccelerator);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_VIDEO_ENCODE_ACCELERATOR_H_
