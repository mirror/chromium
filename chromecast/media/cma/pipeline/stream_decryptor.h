// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_PIPELINE_STREAM_DECRYPTOR_H_
#define CHROMECAST_MEDIA_CMA_PIPELINE_STREAM_DECRYPTOR_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace chromecast {
namespace media {
class DecoderBufferBase;

// Decryptor to get clear buffers in asynchronous way. All the buffers are
// pushed into decryptor to keep the order of frames.
class StreamDecryptor {
 public:
  using PushBufferCompleteCB = base::OnceCallback<void(bool)>;

  StreamDecryptor() = default;
  virtual ~StreamDecryptor() = default;

  // Pushes buffer for decrypting.
  // Implementation should call |push_buffer_complete_cb| when buffer is pushed.
  // New buffers won't be pushed if |push_buffer_complete_cb| isn't invoked.
  // Implementation should cache decrypted buffer and caller will call
  // GetNextReadyBuffer to retrieve it.
  virtual void PushBuffer(scoped_refptr<DecoderBufferBase> buffer,
                          PushBufferCompleteCB push_buffer_complete_cb) = 0;

  // Gets the buffer which is ready to push to decoder. Returns false only if
  // decryption fails. If there's no available buffer now, return nullptr in
  // |buffer|.
  virtual bool GetNextReadyBuffer(scoped_refptr<DecoderBufferBase>* buffer) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(StreamDecryptor);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_PIPELINE_STREAM_DECRYPTOR_H_
