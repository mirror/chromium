// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/filters/offload_video_decoder.h"

namespace media {

VpxVideoDecoder::VpxVideoDecoder() : weak_factory_(this) {
  DETACH_FROM_THREAD(thread_checker_);
}

VpxVideoDecoder::~VpxVideoDecoder() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // release ot offload task_runner_;
}

std::string VpxVideoDecoder::GetDisplayName() const {
  // This call is expected to be static and safe to call from any thread.
  return decoder_->GetDisplayName();
}

void VpxVideoDecoder::Initialize(const VideoDecoderConfig& config,
                                 bool low_delay,
                                 CdmContext* cdm_context,
                                 const InitCB& init_cb,
                                 const OutputCB& output_cb) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(config.IsValidConfig());

  InitCB bound_init_cb = BindToCurrentLoop(init_cb);

  // Encrypted decoding is not supported in software.
  if (config.is_encrypted()) {
    bound_init_cb.Run(false);
    return false;
  }

  // Fail fast if this is not a supported codec.
  if (std::find(supported_codecs_.begin(), supported_codecs_.end(),
                config.codec()) == supported_codecs_.end()) {
    bound_init_cb.Run(false);
    return false;
  }

  // We now need to at least attempt initialize.
  decoder_ = create_decoder_cb.Run();
  if (config.coded_size().width() < 1024) {
    offload_task_runner_ = nullptr;

    return;
  }

  if (config.is_encrypted() || !ConfigureDecoder(config)) {
    bound_init_cb.Run(false);
    return;
  }

  // Success!
  config_ = config;
  state_ = kNormal;
  output_cb_ = offload_task_runner_ ? BindToCurrentLoop(output_cb) : output_cb;
  bound_init_cb.Run(true);
}

void VpxVideoDecoder::Decode(const scoped_refptr<DecoderBuffer>& buffer,
                             const DecodeCB& decode_cb) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(buffer.get());
  DCHECK(!decode_cb.is_null());

  DecodeCB bound_decode_cb = BindToCurrentLoop(decode_cb);

  if (offload_task_runner_) {
    offload_task_runner_->PostTask(
        FROM_HERE, base::Bind(&VpxVideoDecoder::DecodeBuffer,
                              base::Unretained(this), buffer, bound_decode_cb));
  } else {
    DecodeBuffer(buffer, bound_decode_cb);
  }
}

void VpxVideoDecoder::Reset(const base::Closure& closure) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (offload_task_runner_) {
    offload_task_runner_->PostTask(
        FROM_HERE,
        BindToCurrentLoop(base::Bind(&VpxVideoDecoder::ResetHelper,
                                     weak_factory_.GetWeakPtr(), closure)));
    return;
  }

  // BindToCurrentLoop() to avoid calling |closure| inmediately.
  ResetHelper(BindToCurrentLoop(closure));
}

}  // namespace media
