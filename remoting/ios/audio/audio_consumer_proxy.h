// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_AUDIO_AUDIO_CONSUMER_PROXY_H_
#define REMOTING_IOS_AUDIO_AUDIO_CONSUMER_PROXY_H_

#include <stddef.h>
#include <stdint.h>

#include <list>
#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "remoting/proto/audio.pb.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace remoting {

// Takes an instance of |AudioConsumer| and runs it on the |audio_task_runner|
// thread.
class AudioConsumerProxy : public AudioStub {
 public:
  static std::unique_ptr<AudioConsumerProxy> Create(
      scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
      std::unique_ptr<protocol::AudioStub> audio_consumer);

  AudioConsumerProxy();
  ~AudioConsumerProxy() override;

  // AudioConsumer overrides.
  void AddAudioPacket(std::unique_ptr<AudioPacket> packet) override;
  base::WeakPtr<protocol::AudioStub> AudioConsumerAsWeakPtr() override;
  void Stop() override;

 private:
  class Core;

  void Start(Core* core);

  // All thread switching logic is implemented in the ref-counted |Core| class.
  scoped_refptr<Core> core_;

  base::WeakPtrFactory<AudioConsumerProxy> weak_factory_;
};

}  // namespace remoting

#endif  // REMOTING_IOS_AUDIO_AUDIO_CONSUMER_PROXY_H_
