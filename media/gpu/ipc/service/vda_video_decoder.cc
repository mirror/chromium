// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/ipc/service/vda_video_decoder.h"

#include "base/logging.h"

namespace media {

VdaVideoDecoder::VdaVideoDecoder(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner,
    GetStubCB get_stub_cb) {
  DVLOG(1) << __func__;
}

void VdaVideoDecoder::Destroy() {
  delete this;
}

VdaVideoDecoder::~VdaVideoDecoder() {}

std::string VdaVideoDecoder::GetDisplayName() const {
  return "VdaVideoDecoder";
}

void VdaVideoDecoder::Initialize(const VideoDecoderConfig& config,
                                 bool low_delay,
                                 CdmContext* cdm_context,
                                 const InitCB& init_cb,
                                 const OutputCB& output_cb) {}

void VdaVideoDecoder::Decode(const scoped_refptr<DecoderBuffer>& buffer,
                             const DecodeCB& decode_cb) {}

void VdaVideoDecoder::Reset(const base::RepeatingClosure& reset_cb) {}

bool VdaVideoDecoder::NeedsBitstreamConversion() const {
  // TODO(sandersd): Can we do conversion only on this side?
  return false;
}

bool VdaVideoDecoder::CanReadWithoutStalling() const {
  // TODO(sandersd): Base this on picture buffer count.
  return true;
}

int VdaVideoDecoder::GetMaxDecodeRequests() const {
  // TODO(sandersd): Copy from GpuVideoDecoder.
  return 1;
}

void VdaVideoDecoder::NotifyInitializationComplete(bool success) {}

void VdaVideoDecoder::ProvidePictureBuffers(uint32_t requested_num_of_buffers,
                                            VideoPixelFormat format,
                                            uint32_t textures_per_buffer,
                                            const gfx::Size& dimensions,
                                            uint32_t texture_target) {}

void VdaVideoDecoder::DismissPictureBuffer(int32_t picture_buffer_id) {}

void VdaVideoDecoder::PictureReady(const Picture& picture) {}

void VdaVideoDecoder::NotifyEndOfBitstreamBuffer(int32_t bitstream_buffer_id) {}

void VdaVideoDecoder::NotifyFlushDone() {}

void VdaVideoDecoder::NotifyResetDone() {}

void VdaVideoDecoder::NotifyError(VideoDecodeAccelerator::Error error) {}

}  // namespace media
