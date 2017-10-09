// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/audio_service.h"

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "media/audio/audio_manager.h"
#include "media/audio/audio_thread.h"
#include "services/audio/audio_system_info.h"
#include "services/service_manager/public/cpp/service_context.h"

#include "base/debug/stack_trace.h"

namespace audio {

namespace {
enum { kQuiteTimeoutSeconds = 5 };

// Thread class for hosting owned AudioManager on the main thread of the
// service.
class OwnAudioManagerThread : public media::AudioThread {
 public:
  OwnAudioManagerThread();
  ~OwnAudioManagerThread() override;

  // AudioThread implementation.
  void Stop() override;
  base::SingleThreadTaskRunner* GetTaskRunner() override;
  base::SingleThreadTaskRunner* GetWorkerTaskRunner() override;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  THREAD_CHECKER(thread_checker_);
  DISALLOW_COPY_AND_ASSIGN(OwnAudioManagerThread);
};

OwnAudioManagerThread::OwnAudioManagerThread()
    : task_runner_(base::ThreadTaskRunnerHandle::Get()) {}

OwnAudioManagerThread::~OwnAudioManagerThread() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void OwnAudioManagerThread::Stop() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

base::SingleThreadTaskRunner* OwnAudioManagerThread::GetTaskRunner() {
  return task_runner_.get();
}

base::SingleThreadTaskRunner* OwnAudioManagerThread::GetWorkerTaskRunner() {
  NOTREACHED();
  return task_runner_.get();
}

}  // namespace

AudioService::AudioManagerAccessor::AudioManagerAccessor(
    media::AudioManager* audio_manager)
    : audio_manager_(audio_manager) {
  DLOG(ERROR) << "***Accessor";
  DETACH_FROM_SEQUENCE(sequence_checker_);
  DCHECK(audio_manager_);
}

AudioService::AudioManagerAccessor::AudioManagerAccessor(
    AudioManagerFactory audio_manager_factory)
    : audio_manager_factory_(std::move(audio_manager_factory)) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

AudioService::AudioManagerAccessor::~AudioManagerAccessor() {
  if (own_audio_manager_) {
    DCHECK(own_audio_manager_->GetTaskRunner()->BelongsToCurrentThread());
    own_audio_manager_->Shutdown();
  }
}

base::SingleThreadTaskRunner*
AudioService::AudioManagerAccessor::GetMandatoryTaskRunner() {
  return audio_manager_factory_ ? nullptr : audio_manager_->GetTaskRunner();
}

media::AudioManager* AudioService::AudioManagerAccessor::GetAudioManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (audio_manager_)
    return audio_manager_;

  DCHECK(audio_manager_factory_);
  DCHECK(!own_audio_manager_);

  // TODO(olka): pass AudioLogFactory.
  own_audio_manager_ = std::move(*audio_manager_factory_)
                           .Run(std::make_unique<OwnAudioManagerThread>());
  audio_manager_factory_ = base::Optional<AudioManagerFactory>();

  return audio_manager_ = own_audio_manager_.get();
}

AudioService::AudioService(
    std::unique_ptr<AudioManagerAccessor> audio_manager_accessor)
    : audio_manager_accessor_(std::move(audio_manager_accessor)),
      weak_factory_(this) {
  DCHECK(audio_manager_accessor_);
}

AudioService::~AudioService() = default;

void AudioService::OnStart() {
  ref_factory_ =
      std::make_unique<service_manager::ServiceContextRefFactory>(base::Bind(
          &AudioService::MaybeRequestQuitDelayed, base::Unretained(this)));
  registry_.AddInterface<mojom::AudioSystemInfo>(
      base::Bind(&AudioService::BindAudioSystemInfoRequest,
                 base::Unretained(this)),
      audio_manager_accessor_->GetMandatoryTaskRunner());
}

void AudioService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

bool AudioService::OnServiceManagerConnectionLost() {
  // Shutting down owned AudioManager just in case AudioService won't be
  // properly destroyed.
  audio_manager_accessor_.reset();
  return true;
}

void AudioService::MaybeRequestQuitDelayed() {
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&AudioService::MaybeRequestQuit, weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(kQuiteTimeoutSeconds));
}

void AudioService::MaybeRequestQuit() {
  DCHECK(ref_factory_);
  if (ref_factory_->HasNoRefs())
    context()->RequestQuit();
}

void AudioService::BindAudioSystemInfoRequest(
    mojom::AudioSystemInfoRequest request) {
  if (!audio_system_info_)
    audio_system_info_ = std::make_unique<AudioSystemInfo>(
        audio_manager_accessor_->GetAudioManager());

  audio_system_info_->Bind(std::move(request), ref_factory_->CreateRef());
}

}  // namespace audio
