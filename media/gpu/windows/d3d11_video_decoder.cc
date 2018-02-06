// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/windows/d3d11_video_decoder.h"

#include "base/bind.h"
#include "base/callback.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/decoder_buffer.h"
#include "media/base/video_codecs.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame.h"
#include "media/gpu/windows/d3d11_video_decoder_impl.h"

namespace media {

D3D11VideoDecoder::D3D11VideoDecoder(
    scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner,
    base::RepeatingCallback<gpu::CommandBufferStub*()> get_stub_cb)
    : impl_(TaskRunnerBound<D3D11VideoDecoderImpl>::Create(
          std::move(gpu_task_runner),
          get_stub_cb)) {}

D3D11VideoDecoder::~D3D11VideoDecoder() {}

std::string D3D11VideoDecoder::GetDisplayName() const {
  return "D3D11VideoDecoder";
}

void D3D11VideoDecoder::Initialize(const VideoDecoderConfig& config,
                                   bool low_delay,
                                   CdmContext* cdm_context,
                                   const InitCB& init_cb,
                                   const OutputCB& output_cb) {
  bool is_h264 = config.profile() >= H264PROFILE_MIN &&
                 config.profile() <= H264PROFILE_MAX;
  if (!is_h264) {
    init_cb.Run(false);
    return;
  }

  // Bind our own init / output cb that hop to this thread, so we don't call the
  // originals on some other thread.
  // TODO(liberato): what's the lifetime of |cdm_context|?
  impl_.Post(FROM_HERE, &VideoDecoder::Initialize, config, low_delay,
             cdm_context, BindToCurrentLoop(init_cb),
             BindToCurrentLoop(output_cb));
}

void D3D11VideoDecoder::Decode(const scoped_refptr<DecoderBuffer>& buffer,
                               const DecodeCB& decode_cb) {
  impl_.Post(FROM_HERE, &VideoDecoder::Decode, buffer,
             BindToCurrentLoop(decode_cb));
}

void D3D11VideoDecoder::Reset(const base::Closure& closure) {
  impl_.Post(FROM_HERE, &VideoDecoder::Reset, BindToCurrentLoop(closure));
}

bool D3D11VideoDecoder::NeedsBitstreamConversion() const {
  return true;
}

bool D3D11VideoDecoder::CanReadWithoutStalling() const {
  return false;
}

int D3D11VideoDecoder::GetMaxDecodeRequests() const {
  return 4;
}

}  // namespace media
