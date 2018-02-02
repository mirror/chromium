// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/pipeline/cdm_decryptor.h"

#include "base/bind.h"
#include "chromecast/media/base/decrypt_context_impl.h"
#include "chromecast/media/cma/base/decoder_buffer_base.h"
#include "chromecast/media/cma/pipeline/decrypt_util.h"

namespace chromecast {
namespace media {

CdmDecryptor::CdmDecryptor()
    : ready_buffer_(nullptr), decrypt_success_(true), weak_factory_(this) {
  weak_this_ = weak_factory_.GetWeakPtr();
}

CdmDecryptor::~CdmDecryptor() = default;

void CdmDecryptor::PushBuffer(scoped_refptr<DecoderBufferBase> buffer,
                              PushBufferCompleteCB push_buffer_complete_cb) {
  DCHECK(push_buffer_complete_cb_.is_null());
  push_buffer_complete_cb_ = std::move(push_buffer_complete_cb);

  if (buffer->end_of_stream() || !buffer->decrypt_config()) {
    OnResult(buffer, true);
    return;
  }

  DecryptContextImpl* decrypt_context =
      static_cast<DecryptContextImpl*>(buffer->decrypt_context());
  DCHECK(decrypt_context);

  if (decrypt_context->CanDecryptToBuffer()) {
    DecryptDecoderBuffer(buffer, decrypt_context,
                         base::BindOnce(&CdmDecryptor::OnResult, weak_this_));
    return;
  }

  // Media pipeline backend will handle decryption.
  OnResult(buffer, true);
}

bool CdmDecryptor::GetNextReadyBuffer(
    scoped_refptr<DecoderBufferBase>* buffer) {
  if (!decrypt_success_)
    return false;

  *buffer = std::move(ready_buffer_);
  return true;
}

void CdmDecryptor::OnResult(scoped_refptr<DecoderBufferBase> buffer,
                            bool success) {
  DCHECK(!ready_buffer_);
  decrypt_success_ &= success;
  ready_buffer_ = std::move(buffer);

  std::move(push_buffer_complete_cb_).Run(true);
}

}  // namespace media
}  // namespace chromecast
