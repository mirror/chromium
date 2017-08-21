// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_AUDIO_DECODE_SCHEDULER_H_
#define REMOTING_CLIENT_AUDIO_DECODE_SCHEDULER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "remoting/protocol/audio_stub.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace remoting {

namespace protocol {
class SessionConfig;
}  // namespace protocol

class AudioPacket;
class AudioBuffer;

class AudioDecodeScheduler : public protocol::AudioStub {
 public:
  AudioDecodeScheduler(
      scoped_refptr<base::SingleThreadTaskRunner> main_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> audio_decode_task_runner,
      std::unique_ptr<AudioBuffer> audio_buffer);
  ~AudioDecodeScheduler() override;

  // Initializes decoder with the information from the protocol config.
  void Initialize(const protocol::SessionConfig& config);

  // AudioStub implementation.
  void ProcessAudioPacket(std::unique_ptr<AudioPacket> packet,
                          const base::Closure& done) override;

 private:
  class Core;

  std::unique_ptr<Core> core_;

  DISALLOW_COPY_AND_ASSIGN(AudioDecodeScheduler);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_AUDIO_DECODE_SCHEDULER_H_
