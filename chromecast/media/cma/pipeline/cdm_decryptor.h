// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_PIPELINE_CDM_DECRYPTOR_H_
#define CHROMECAST_MEDIA_CMA_PIPELINE_CDM_DECRYPTOR_H_

#include "base/memory/weak_ptr.h"
#include "chromecast/media/cma/pipeline/stream_decryptor.h"

namespace chromecast {
namespace media {

// StreamDecryptor implemented with CDM decrypt APIs.
class CdmDecryptor : public StreamDecryptor {
 public:
  CdmDecryptor();
  ~CdmDecryptor() override;

  // StreamDecryptor implementation:
  void PushBuffer(scoped_refptr<DecoderBufferBase> buffer,
                  PushBufferCompleteCB push_buffer_complete_cb) override;
  bool GetNextReadyBuffer(scoped_refptr<DecoderBufferBase>* buffer) override;

 private:
  void OnResult(scoped_refptr<DecoderBufferBase> buffer, bool success);

  PushBufferCompleteCB push_buffer_complete_cb_;
  scoped_refptr<DecoderBufferBase> ready_buffer_;
  bool decrypt_success_;

  base::WeakPtr<CdmDecryptor> weak_this_;
  base::WeakPtrFactory<CdmDecryptor> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(CdmDecryptor);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_PIPELINE_CDM_DECRYPTOR_H_
