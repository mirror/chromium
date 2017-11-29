// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/audio_service.h"

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "media/audio/audio_manager.h"
#include "services/audio/audio_system_info.h"
//#include "services/audio/in_process_audio_manager_accessor.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace audio {

AudioService::AudioService(
    std::unique_ptr<AudioManagerAccessor> audio_manager_accessor)
    : audio_manager_accessor_(std::move(audio_manager_accessor)) {
  DCHECK(audio_manager_accessor_);
}

AudioService::~AudioService() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void AudioService::OnStart() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  registry_.AddInterface<mojom::AudioSystemInfo>(base::Bind(
      &AudioService::BindAudioSystemInfoRequest, base::Unretained(this)));
}

void AudioService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

bool AudioService::OnServiceManagerConnectionLost() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // Shutting down owned AudioManager, in case we own one.
  audio_manager_accessor_.reset();
  return true;
}

void AudioService::BindAudioSystemInfoRequest(
    mojom::AudioSystemInfoRequest request) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!audio_system_info_) {
    audio_system_info_ = std::make_unique<AudioSystemInfo>(
        audio_manager_accessor_->GetAudioManager());
  }
  audio_system_info_->Bind(std::move(request));
}

}  // namespace audio
