// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/owning_audio_manager_accessor.h"

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "media/audio/audio_manager.h"
#include "media/audio/audio_thread.h"

namespace audio {

namespace {

// Thread class for hosting owned AudioManager on the main thread of the
// service.
class OwnedAudioManagerThread : public media::AudioThread {
 public:
  OwnedAudioManagerThread();
  ~OwnedAudioManagerThread() override;

  // AudioThread implementation.
  void Stop() override;
  base::SingleThreadTaskRunner* GetTaskRunner() override;
  base::SingleThreadTaskRunner* GetWorkerTaskRunner() override;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  DISALLOW_COPY_AND_ASSIGN(OwnedAudioManagerThread);
};

OwnedAudioManagerThread::OwnedAudioManagerThread()
    : task_runner_(base::ThreadTaskRunnerHandle::Get()) {}

OwnedAudioManagerThread::~OwnedAudioManagerThread() {
  DCHECK(task_runner_->BelongsToCurrentThread());
}

void OwnedAudioManagerThread::Stop() {
  DCHECK(task_runner_->BelongsToCurrentThread());
}

base::SingleThreadTaskRunner* OwnedAudioManagerThread::GetTaskRunner() {
  return task_runner_.get();
}

base::SingleThreadTaskRunner* OwnedAudioManagerThread::GetWorkerTaskRunner() {
  // TODO(olka): implement when moving stream factories into the service.
  NOTREACHED();
  return task_runner_.get();
}

}  // namespace

OwningAudioManagerAccessor::OwningAudioManagerAccessor(
    AudioManagerFactoryCallback audio_manager_factory_cb)
    : audio_manager_factory_cb_(std::move(audio_manager_factory_cb)) {
  DCHECK(audio_manager_factory_cb_);
}

OwningAudioManagerAccessor::~OwningAudioManagerAccessor() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (audio_manager_)
    audio_manager_->Shutdown();
}

media::AudioManager* OwningAudioManagerAccessor::GetAudioManager() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!audio_manager_) {
    DCHECK(audio_manager_factory_cb_);
    // TODO(olka): pass AudioLogFactory.
    audio_manager_ = base::ResetAndReturn(&audio_manager_factory_cb_)
                         .Run(std::make_unique<OwnedAudioManagerThread>());
    DCHECK(audio_manager_);
  }
  DCHECK(audio_manager_->GetTaskRunner()->BelongsToCurrentThread());
  return audio_manager_.get();
}

}  // namespace audio
