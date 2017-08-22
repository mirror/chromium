// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_AUDIO_SUPPLIER_H_
#define REMOTING_CLIENT_AUDIO_SUPPLIER_H_

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "remoting/proto/audio.pb.h"

namespace remoting {

class AudioSupplier {
 public:
  AudioSupplier() {}
  virtual ~AudioSupplier() {}
  virtual uint32_t GetAudioFrame(void* buffer, uint32_t buffer_size) = 0;

  // Methods to describe buffered data.
  virtual AudioPacket::SamplingRate GetBufferedSamplingRate() = 0;
  virtual AudioPacket::Channels GetBufferedChannels() = 0;
  virtual AudioPacket::BytesPerSample GetBufferedByesPerSample() = 0;
  virtual uint32_t GetBytesPerFrame() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioSupplier);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_AUDIO_SUPPLIER_H_
