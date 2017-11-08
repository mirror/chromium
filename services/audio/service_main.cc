// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_manager.h"
#include "media/audio/audio_thread_impl.h"
#include "services/audio/audio_service.h"
#include "services/audio/owning_audio_manager_accessor.h"
#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/service_runner.h"

namespace {

std::unique_ptr<media::AudioManager> CreateAudioManager(
    std::unique_ptr<media::AudioThread> audio_thread) {
  // TODO(olka): pass the log.
  return media::AudioManager::Create(std::move(audio_thread), nullptr);
}

}  // namespace

MojoResult ServiceMain(MojoHandle service_request_handle) {
  DLOG(ERROR) << "****ServiceMain";
  return service_manager::ServiceRunner(
             new audio::AudioService(
                 std::make_unique<audio::OwningAudioManagerAccessor>(
                     base::BindOnce(&CreateAudioManager)),
                 base::TimeDelta::FromSeconds(10)))
      .Run(service_request_handle);
}
