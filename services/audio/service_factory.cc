// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/service_factory.h"

#include "services/audio/in_process_audio_manager_accessor.h"
#include "services/audio/service.h"

namespace audio {

namespace service_factory {

std::unique_ptr<service_manager::Service> CreateInProcess(
    media::AudioManager* audio_manager) {
  return std::make_unique<Service>(
      std::make_unique<InProcessAudioManagerAccessor>(audio_manager));
}

}  // namespace service_factory

}  // namespace audio
