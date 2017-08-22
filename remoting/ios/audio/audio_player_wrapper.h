// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_AUDIO_AUDIO_PLAYER_WRAPPER_H_
#define REMOTING_IOS_AUDIO_AUDIO_PLAYER_WRAPPER_H_

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <memory>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "remoting/client/audio_consumer.h"
#include "remoting/proto/audio.pb.h"

namespace remoting {

class AudioSupplier;

class AudioPlayerWrapper : public AudioConsumer {
 public:
  AudioPlayerWrapper();
  ~AudioPlayerWrapper() override;
  void AddAudioPacket(std::unique_ptr<AudioPacket> packet) override;

 private:
  protocol::AudioStub* audio_consumer_;
  AudioSupplier* audio_supplier_;

  DISALLOW_COPY_AND_ASSIGN(AudioPlayerWrapper);
};

}  // namespace remoting

#endif  // REMOTING_IOS_AUDIO_AUDIO_PLAYER_WRAPPER_H_
