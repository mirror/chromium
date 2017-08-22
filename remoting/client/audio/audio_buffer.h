// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_AUDIO_AUDIO_BUFFER_H_
#define REMOTING_CLIENT_AUDIO_AUDIO_BUFFER_H_

#include <stddef.h>
#include <stdint.h>

#include <list>

#include <memory>
#include "base/callback.h"
#include "base/macros.h"
#include "remoting/proto/audio.pb.h"

namespace remoting {

typedef base::Callback<void(void* context)> AudioBufferPopCallback;

// This class provides an interface for buffering audio packets.
// Audio Pipeline Context:
// Stream -> Decode -> Stream Consumer -> [Buffer] -> Frame Provider -> Play
class AudioBuffer {
 public:
  virtual void Push(std::unique_ptr<AudioPacket> packet) = 0;
  virtual uint32_t Pop(void* samples, uint32_t buffer_size) = 0;
  virtual void Pop(void* samples,
                   uint32_t buffer_size,
                   void* context,
                   const AudioBufferPopCallback& callback) = 0;
  virtual void Reset() = 0;
  virtual uint32_t Size() = 0;

  // Methods to describe buffered data.
  virtual AudioPacket::SamplingRate GetSamplingRate();
  virtual AudioPacket::Channels GetChannels();
  virtual AudioPacket::BytesPerSample GetByesPerSample();
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_AUDIO_AUDIO_BUFFER_H_
