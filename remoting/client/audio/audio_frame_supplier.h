// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_AUDIO_AUDIO_FRAME_SUPPLIER_H_
#define REMOTING_CLIENT_AUDIO_AUDIO_FRAME_SUPPLIER_H_

#include <stdint>

#include "base/macros.h"
#include "remoting/proto/audio.pb.h"

namespace remoting {

// This class provides an interface to request audio frames of a given size
// and a way to ask about the currently buffered audio data.
// Audio Pipeline Context:
// Stream -> Decode -> Stream Consumer -> Buffer -> [Frame Supplier] -> Play
class AudioFrameSupplier {
 public:
  AudioFrameSupplier() = default;
  virtual ~AudioFrameSupplier() = default;
  virtual uint32_t GetAudioFrame(void* buffer, uint32_t buffer_size) = 0;

  // Methods to describe buffered data.
  virtual const AudioPacket::SamplingRate buffered_sampling_rate() = 0;
  virtual const AudioPacket::Channels buffered_channels() = 0;
  virtual const AudioPacket::BytesPerSample buffered_byes_per_sample() = 0;
  virtual const uint32_t bytes_per_frame() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioFrameSupplier);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_AUDIO_AUDIO_FRAME_SUPPLIER_H_
