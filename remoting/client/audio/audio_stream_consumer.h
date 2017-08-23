// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_AUDIO_AUDIO_STREAM_CONSUMER_H_
#define REMOTING_CLIENT_AUDIO_AUDIO_STREAM_CONSUMER_H_

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "remoting/proto/audio.pb.h"
#include "remoting/protocol/audio_stub.h"

namespace remoting {

// This is the interface that consumes the audio stream from the decoder.
// The decoder is not guaranteed to produce a constaint flow of audio, this
// interface sits between the audio decoder and the audio frame provider.
// Audio Pipeline Context:
// Stream -> Decode -> [Stream Consumer] -> Buffer -> Frame Provider -> Play
class AudioStreamConsumer : public protocol::AudioStub {
 public:
  AudioStreamConsumer() {}
  virtual ~AudioStreamConsumer() {}
  virtual void AddAudioPacket(std::unique_ptr<AudioPacket> packet) = 0;
  virtual void Stop() = 0;
  virtual base::WeakPtr<AudioConsumer> AudioConsumerAsWeakPtr() = 0;

  // AudioStub implementation. Deprecated. Delegates to AddAudioPacket.
  void ProcessAudioPacket(std::unique_ptr<AudioPacket> packet,
                          const base::Closure& done) override;

 private:
  friend class AudioStreamConsumerProxy;
  DISALLOW_COPY_AND_ASSIGN(AudioStreamConsumer);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_AUDIO_AUDIO_STREAM_CONSUMER_H_
