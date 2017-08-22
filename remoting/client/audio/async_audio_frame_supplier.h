// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_ASYNC_AUDIO_FRAME_SUPPLIER_H_
#define REMOTING_CLIENT_ASYNC_AUDIO_FRAME_SUPPLIER_H_

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "remoting/client/audio/audio_frame_supplier.h"
#include "remoting/proto/audio.pb.h"

namespace remoting {

typedef base::Callback<void(const void* context, const void* samples)>
    AudioBufferGetAudioFrameCallback;

// This interface extends the AudioFrameSupplier interface adding async support
// for audio frame requests. This allows the audio frame supplier to wait until
// a full frame is buffered before returning the audio frame to the caller.
// Audio Pipeline Context:
// Stream -> Decode -> Stream Consumer -> Buffer -> [Frame Provider] -> Play
class AsyncAudioFrameSupplier : public AudioFrameSupplier {
 public:
  virtual void AsyncGetAudioFrame(
      void* samples,
      uint32_t buffer_size,
      void* context,
      const AudioBufferGetAudioFrameCallback& callback) = 0;
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_ASYNC_AUDIO_SUPPLIER_H_
