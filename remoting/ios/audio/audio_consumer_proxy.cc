// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/ios/audio/audio_consumer_proxy.h"

#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"

namespace remoting {

// Runs an instance of |HostWindow| on the |audio_task_runner_| thread.
class AudioConsumerProxy::Core : public base::RefCountedThreadSafe<Core> {
 public:
  Core(scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
       scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
       std::unique_ptr<protocol::AudioStub> audio_consumer);

  void AddAudioPacket(std::unique_ptr<AudioPacket> packet);
  void Stop();
  base::WeakPtr<protocol::AudioStub> AudioConsumerAsWeakPtr();
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

  // The wrapped |AudioConsumer| instance running on the |audio_task_runner_|
  // thread.
  std::unique_ptr<protocol::AudioStub> audio_consumer_;

  // Used to create the control pointer passed to |audio_consumer_|.
  // base::WeakPtrFactory<ClientSessionControl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Core);
};

std::unique_ptr<AudioConsumerProxy> AudioConsumerProxy::Create(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
    std::unique_ptr<protocol::AudioStub> audio_consumer) {
  std::unique_ptr<AudioConsumerProxy> result(new AudioConsumerProxy());
  Core* core = new Core(caller_task_runner, audio_task_runner,
                        std::move(audio_consumer));
  result->Start(core);
  return result;
}

AudioConsumerProxy::AudioConsumerProxy() : weak_factory_(this) {}

AudioConsumerProxy::~AudioConsumerProxy() {}

void AudioConsumerProxy::AddAudioPacket(std::unique_ptr<AudioPacket> packet) {
  core_->AddAudioPacket(std::move(packet));
}

void AudioConsumerProxy::Start(Core* core) {
  core_ = core;
}

void AudioConsumerProxy::Stop() {
  core_->Stop();
}

base::WeakPtr<AudioConsumer> AudioConsumerProxy::AudioConsumerAsWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

AudioConsumerProxy::Core::Core(
    scoped_refptr<base::SingleThreadTaskRunner> caller_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> audio_task_runner,
    std::unique_ptr<AudioConsumer> audio_consumer)
    : caller_task_runner_(caller_task_runner),
      audio_task_runner_(audio_task_runner) {
  //      weak_factory_(this) {
  audio_consumer_ = std::move(audio_consumer);
}

AudioConsumerProxy::Core::~Core() {}

void AudioConsumerProxy::Core::AddAudioPacket(
    std::unique_ptr<AudioPacket> packet) {
  audio_task_runner_->PostTask(
      FROM_HERE, base::Bind(&Core::AddAudioPacketOnAudioThread, this,
                            base::Passed(&packet)));
}

void AudioConsumerProxy::Core::Stop() {
  audio_task_runner_->PostTask(FROM_HERE,
                               base::Bind(&Core::StopOnAudioThread, this));
}

void AudioConsumerProxy::Core::Detach() {}

void AudioConsumerProxy::Core::AddAudioPacketOnAudioThread(
    std::unique_ptr<AudioPacket> packet) {
  DCHECK(audio_task_runner_->BelongsToCurrentThread());

  audio_consumer_->AddAudioPacket(std::move(packet));
}

void AudioConsumerProxy::Core::StopOnAudioThread() {
  DCHECK(audio_task_runner_->BelongsToCurrentThread());

  audio_consumer_->Stop();
}

}  // namespace remoting
