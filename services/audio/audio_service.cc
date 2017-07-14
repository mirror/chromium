// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/audio_service.h"

//#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "media/audio/audio_manager.h"
#include "media/audio/audio_thread_impl.h"
#include "services/audio/audio_system_info.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace audio {

namespace {
enum { kQuiteTimeoutSeconds = 5 };
}  // namespace

AudioService::AudioService() : weak_factory_(this) {}

AudioService::~AudioService() {
  // if (audio_manager_)
  //   audio_manager_->Shutdown();
}

// static
std::unique_ptr<service_manager::Service> AudioService::Create() {
  return base::MakeUnique<AudioService>();
}

void AudioService::OnStart() {
  ref_factory_.reset(new service_manager::ServiceContextRefFactory(base::Bind(
      &AudioService::MaybeRequestQuitDelayed, base::Unretained(this))));
  registry_.AddInterface(base::Bind(&AudioService::BindAudioSystemInfoRequest,
                                    base::Unretained(this)));

  // context()->SetQuiteClosure(base::Bind(&AudioService::Shutdown,
  // base::Unretained(this));
}

void AudioService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(source_info, interface_name,
                          std::move(interface_pipe));
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
    const service_manager::BindSourceInfo& source_info,
    mojom::AudioSystemInfoRequest request) {
  if (!audio_system_info_)
    audio_system_info_ = AudioSystemInfo::Create(LazyGetAudioManager());
  audio_system_info_->Bind(std::move(request), ref_factory_->CreateRef());
}

media::AudioManager* AudioService::LazyGetAudioManager() {
  return media::AudioManager::Get();
  /*
  //TODO thread checker
   if (audio_manager_)
    return audio_manager_.get();

  // TODO(olka) Allow to inject embedder factory for AudioManager.
  // TODO(olka) Implement AudioLogFactory on top of Mojo client notifications.
  audio_manager_ = media::AudioManager::Create(
      base::MakeUnique<media::AudioThreadImpl>(), nullptr);
  CHECK(audio_manager_);

  return audio_manager_.get();
  */
}

// OnServiceManagerConnectionLost()

// void AudioService::Shutdown() {
//  if (audio_manager_)
//    audio_manager_->Shutdown();
//}

}  // namespace audio
