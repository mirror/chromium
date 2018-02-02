// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_PIPELINE_BACKEND_DECRYPTOR_H_
#define CHROMECAST_MEDIA_CMA_PIPELINE_BACKEND_DECRYPTOR_H_

#include <memory>
#include <queue>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chromecast/media/cma/base/decoder_buffer_base.h"
#include "chromecast/media/cma/pipeline/stream_decryptor.h"
#include "chromecast/public/media/media_pipeline_backend.h"

namespace chromecast {
namespace media {

class BackendDecryptor : public StreamDecryptor,
                         MediaPipelineBackend::AudioDecryptor::Delegate {
 public:
  BackendDecryptor(
      std::unique_ptr<MediaPipelineBackend::AudioDecryptor> decryptor);
  ~BackendDecryptor() override;

  // StreamDecryptor implementation:
  void PushBuffer(scoped_refptr<DecoderBufferBase> buffer,
                  PushBufferCompleteCB push_buffer_complete_cb) override;
  bool GetNextReadyBuffer(scoped_refptr<DecoderBufferBase>* buffer) override;

 private:
  // MediaPipelineBackend::AudioDecryptor::Delegate implementation:
  void OnPushBufferComplete(MediaPipelineBackend::BufferStatus status) override;
  void OnDecryptComplete(bool success) override;

  // Pending buffers for decrypt.
  std::queue<scoped_refptr<DecoderBufferBase>> pending_buffers_;

  // Buffers that are ready to return to caller.
  std::queue<scoped_refptr<DecoderBufferBase>> ready_buffers_;
  bool decrypt_success_;

  std::unique_ptr<MediaPipelineBackend::AudioDecryptor> decryptor_;

  PushBufferCompleteCB push_buffer_complete_cb_;

  DISALLOW_COPY_AND_ASSIGN(BackendDecryptor);
};

}  // namespace media
}  // namespace chromecast
#endif  // CHROMECAST_MEDIA_CMA_PIPELINE_BACKEND_DECRYPTOR_H_
