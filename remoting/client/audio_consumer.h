// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_AUDIO_CONSUMER_H_
#define REMOTING_CLIENT_AUDIO_CONSUMER_H_

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "remoting/proto/audio.pb.h"

namespace remoting {

class AudioConsumer {
 public:
  AudioConsumer() {}
  virtual ~AudioConsumer() {}
  virtual void AddAudioPacket(std::unique_ptr<AudioPacket> packet) = 0;
  virtual void Stop() = 0;
  virtual base::WeakPtr<AudioConsumer> AudioConsumerAsWeakPtr() = 0;

 private:
  friend class AudioConsumerProxy;
  DISALLOW_COPY_AND_ASSIGN(AudioConsumer);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_AUDIO_CONSUMER_H_
