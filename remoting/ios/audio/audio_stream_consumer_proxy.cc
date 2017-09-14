// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/ios/audio/audio_stream_consumer_proxy.h"

#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"

namespace remoting {

// Runs an instance of |HostWindow| on the |audio_task_runner_| thread.
class AudioStreamConsumerProxy::Core {
 public:
  Core(scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
       scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
       base::WeakPtr<AudioStreamConsumer> audio_stream_consumer);
  ~Core();

  void AddAudioPacket(std::unique_ptr<AudioPacket> packet);
  void Stop();
  base::WeakPtr<AudioStreamConsumer> AudioStreamConsumerAsWeakPtr();

 private:
  // Start() and Stop() equivalents called on the |audio_task_runner_| thread.
  void AddAudioPacketOnAudioThread(std::unique_ptr<AudioPacket> packet);

  void StopOnAudioThread();

  // Task runner on which public methods of this class must be called.
  scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner_;

  // Task runner on which |host_window_| is running.
  scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner_;

  // The wrapped |AudioStreamConsumer| instance running on the
  // |audio_task_runner_| thread.
  base::WeakPtr<AudioStreamConsumer> audio_stream_consumer_;

  DISALLOW_COPY_AND_ASSIGN(Core);
};

std::unique_ptr<AudioStreamConsumerProxy> AudioStreamConsumerProxy::Create(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
    base::WeakPtr<AudioStreamConsumer> audio_stream_consumer) {
  std::unique_ptr<Core> core(new Core(caller_task_runner, audio_task_runner,
                                      std::move(audio_stream_consumer)));
  std::unique_ptr<AudioStreamConsumerProxy> result(
      new AudioStreamConsumerProxy(std::move(core)));
  return result;
}

AudioStreamConsumerProxy::AudioStreamConsumerProxy(std::unique_ptr<Core> core)
    : core_(std::move(core)), weak_factory_(this) {}

AudioStreamConsumerProxy::~AudioStreamConsumerProxy() {}

void AudioStreamConsumerProxy::AddAudioPacket(
    std::unique_ptr<AudioPacket> packet) {
  core_->AddAudioPacket(std::move(packet));
}

// void AudioStreamConsumerProxy::Stop() {
//  core_->Stop();
//}

base::WeakPtr<AudioStreamConsumer>
AudioStreamConsumerProxy::AudioStreamConsumerAsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

AudioStreamConsumerProxy::Core::Core(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
    base::WeakPtr<AudioStreamConsumer> audio_stream_consumer)
    : caller_task_runner_(caller_task_runner),
      audio_task_runner_(audio_task_runner),
      audio_stream_consumer_(audio_stream_consumer) {}

AudioStreamConsumerProxy::Core::~Core() {}

void AudioStreamConsumerProxy::Core::AddAudioPacket(
    std::unique_ptr<AudioPacket> packet) {
  audio_task_runner_->PostTask(
      FROM_HERE, base::Bind(&Core::AddAudioPacketOnAudioThread,
                            base::Unretained(this), base::Passed(&packet)));
}

void AudioStreamConsumerProxy::Core::Stop() {
  audio_task_runner_->PostTask(
      FROM_HERE, base::Bind(&Core::StopOnAudioThread, base::Unretained(this)));
}

void AudioStreamConsumerProxy::Core::AddAudioPacketOnAudioThread(
    std::unique_ptr<AudioPacket> packet) {
  DCHECK(audio_task_runner_->BelongsToCurrentThread());

  audio_stream_consumer_->AddAudioPacket(std::move(packet));
}

void AudioStreamConsumerProxy::Core::StopOnAudioThread() {
  DCHECK(audio_task_runner_->BelongsToCurrentThread());

  // audio_stream_consumer_->Stop();
}

}  // namespace remoting
