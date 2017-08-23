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
class AudioStreamConsumerProxy::Core : public base::RefCountedThreadSafe<Core> {
 public:
  Core(scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
       scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
       std::unique_ptr<AudioStreamConsumer> audio_stream_consumer);

  void AddAudioPacket(std::unique_ptr<AudioPacket> packet);
  void Stop();
  base::WeakPtr<AudioStreamConsumer> AudioStreamConsumerAsWeakPtr();
  void Detach();

 private:
  friend class base::RefCountedThreadSafe<Core>;
  ~Core();

  // Start() and Stop() equivalents called on the |audio_task_runner_| thread.
  void AddAudioPacketOnAudioThread(std::unique_ptr<AudioPacket> packet);

  void StopOnAudioThread();

  // Task runner on which public methods of this class must be called.
  scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner_;

  // Task runner on which |host_window_| is running.
  scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner_;

  // The wrapped |AudioStreamConsumer| instance running on the
  // |audio_task_runner_| thread.
  std::unique_ptr<AudioStreamConsumer> audio_stream_consumer_;

  // Used to create the control pointer passed to |audio_stream_consumer_|.
  // base::WeakPtrFactory<ClientSessionControl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Core);
};

std::unique_ptr<AudioStreamConsumerProxy> AudioStreamConsumerProxy::Create(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
    std::unique_ptr<AudioStreamConsumer> audio_stream_consumer) {
  std::unique_ptr<AudioStreamConsumerProxy> result(
      new AudioStreamConsumerProxy());
  Core* core = new Core(caller_task_runner, audio_task_runner,
                        std::move(audio_stream_consumer));
  result->Start(core);
  return result;
}

AudioStreamConsumerProxy::AudioStreamConsumerProxy() : weak_factory_(this) {}

AudioStreamConsumerProxy::~AudioStreamConsumerProxy() {}

void AudioStreamConsumerProxy::AddAudioPacket(
    std::unique_ptr<AudioPacket> packet) {
  core_->AddAudioPacket(std::move(packet));
}

void AudioStreamConsumerProxy::Start(Core* core) {
  core_ = core;
}

void AudioStreamConsumerProxy::Stop() {
  core_->Stop();
}

base::WeakPtr<AudioStreamConsumer>
AudioStreamConsumerProxy::AudioStreamConsumerAsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

AudioStreamConsumerProxy::Core::Core(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
    std::unique_ptr<AudioStreamConsumer> audio_stream_consumer)
    : caller_task_runner_(caller_task_runner),
      audio_task_runner_(audio_task_runner) {
  //      weak_factory_(this) {
  audio_stream_consumer_ = std::move(audio_stream_consumer);
}

AudioStreamConsumerProxy::Core::~Core() {}

void AudioStreamConsumerProxy::Core::AddAudioPacket(
    std::unique_ptr<AudioPacket> packet) {
  audio_task_runner_->PostTask(
      FROM_HERE, base::Bind(&Core::AddAudioPacketOnAudioThread, this,
                            base::Passed(&packet)));
}

void AudioStreamConsumerProxy::Core::Stop() {
  audio_task_runner_->PostTask(FROM_HERE,
                               base::Bind(&Core::StopOnAudioThread, this));
}

void AudioStreamConsumerProxy::Core::Detach() {}

void AudioStreamConsumerProxy::Core::AddAudioPacketOnAudioThread(
    std::unique_ptr<AudioPacket> packet) {
  DCHECK(audio_task_runner_->BelongsToCurrentThread());

  audio_stream_consumer_->AddAudioPacket(std::move(packet));
}

void AudioStreamConsumerProxy::Core::StopOnAudioThread() {
  DCHECK(audio_task_runner_->BelongsToCurrentThread());

  audio_stream_consumer_->Stop();
}

}  // namespace remoting
