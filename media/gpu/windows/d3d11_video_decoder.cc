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

namespace {

// Check |weak_ptr| and run |cb| with |args| if it's non-null.
template <typename T, typename... Args>
void CallbackOnProperThread(base::WeakPtr<T> weak_ptr,
                            base::Callback<void(Args...)> cb,
                            Args... args) {
  if (weak_ptr.get())
    cb.Run(args...);
}

// Given a callback, |cb|, return another callback that will call |cb| after
// switching to the thread that BindToCurrent.... is called on.  We will check
// |weak_ptr| on the current thread.  This is different than just calling
// BindToCurrentLoop because we'll check the weak ptr.  If |cb| is some method
// of |T|, then one can use BindToCurrentLoop directly.  However, in our case,
// we have some unrelated callback that we'd like to call only if we haven't
// been destroyed yet.  I suppose this could also just be a method:
// template<CB, ...> D3D11VideoDecoder::CallSomeCallback(CB, ...) that's bound
// via BindToCurrentLoop directly.
template <typename T, typename... Args>
base::Callback<void(Args...)> BindToCurrentThreadIfWeakPtr(
    base::WeakPtr<T> weak_ptr,
    base::Callback<void(Args...)> cb) {
  return media::BindToCurrentLoop(
      base::Bind(&CallbackOnProperThread<T, Args...>, weak_ptr, cb));
}

}  // namespace

namespace media {

D3D11VideoDecoder::D3D11VideoDecoder(
    scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner,
    base::RepeatingCallback<gpu::CommandBufferStub*()> get_stub_cb)
    : impl_(std::move(gpu_task_runner), get_stub_cb), weak_factory_(this) {}

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
  impl_.Post(
      FROM_HERE, &VideoDecoder::Initialize, config, low_delay, cdm_context,
      BindToCurrentThreadIfWeakPtr(weak_factory_.GetWeakPtr(), init_cb),
      BindToCurrentThreadIfWeakPtr(weak_factory_.GetWeakPtr(), output_cb));
}

void D3D11VideoDecoder::Decode(const scoped_refptr<DecoderBuffer>& buffer,
                               const DecodeCB& decode_cb) {
  impl_.Post(
      FROM_HERE, &VideoDecoder::Decode, buffer,
      BindToCurrentThreadIfWeakPtr(weak_factory_.GetWeakPtr(), decode_cb));
}

void D3D11VideoDecoder::Reset(const base::Closure& closure) {
  impl_.Post(FROM_HERE, &VideoDecoder::Reset,
             BindToCurrentThreadIfWeakPtr(weak_factory_.GetWeakPtr(), closure));
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
