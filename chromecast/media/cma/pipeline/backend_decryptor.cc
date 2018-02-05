// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/pipeline/backend_decryptor.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "chromecast/base/task_runner_impl.h"
#include "chromecast/media/cma/pipeline/decrypt_util.h"

namespace chromecast {
namespace media {

BackendDecryptor::BackendDecryptor(const EncryptionScheme& scheme)
    : decrypt_success_(true) {
  DCHECK(MediaPipelineBackend::CreateAudioDecryptor);

  task_runner_ = std::make_unique<TaskRunnerImpl>();
  decryptor_.reset(
      MediaPipelineBackend::CreateAudioDecryptor(scheme, task_runner_.get()));

  DCHECK(decryptor_);
  decryptor_->SetDelegate(this);
}

BackendDecryptor::~BackendDecryptor() = default;

void BackendDecryptor::Init(
    const PushBufferCompleteCB& push_buffer_complete_cb) {
  DCHECK(!push_buffer_complete_cb_);
  push_buffer_complete_cb_ = push_buffer_complete_cb;
}

void BackendDecryptor::PushBuffer(scoped_refptr<DecoderBufferBase> buffer) {
  // Push both clear and encrypted buffers to backend, so that |decryptor_|
  // won't be blocked if there're not enough encrypted buffers. EOS buffer is
  // also needed so that the last buffer can be flushed.
  pending_buffers_.push(buffer);

  MediaPipelineBackend::BufferStatus status = decryptor_->PushBufferForDecrypt(
      buffer.get(),
      buffer->end_of_stream() ? nullptr : buffer->writable_data());

  if (status != MediaPipelineBackend::kBufferPending)
    OnPushBufferComplete(status);
}

bool BackendDecryptor::GetNextReadyBuffer(
    scoped_refptr<DecoderBufferBase>* buffer) {
  if (!decrypt_success_)
    return false;

  if (ready_buffers_.empty()) {
    *buffer = nullptr;
    return true;
  }

  *buffer = std::move(ready_buffers_.front());
  ready_buffers_.pop();
  return true;
}

void BackendDecryptor::OnPushBufferComplete(
    MediaPipelineBackend::BufferStatus status) {
  DCHECK(push_buffer_complete_cb_);
  push_buffer_complete_cb_.Run(status == MediaPipelineBackend::kBufferSuccess);
}

void BackendDecryptor::OnDecryptComplete(bool success) {
  DCHECK(!pending_buffers_.empty());

  decrypt_success_ &= success;

  scoped_refptr<DecoderBufferBase> buffer = std::move(pending_buffers_.front());
  pending_buffers_.pop();

  if (!decrypt_success_)
    return;

  ready_buffers_.push(buffer->end_of_stream() || !buffer->decrypt_config()
                          ? buffer
                          : new DecoderBufferClear(buffer));
}

}  // namespace media
}  // namespace chromecast
