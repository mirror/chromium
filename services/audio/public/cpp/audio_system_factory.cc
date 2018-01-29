// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/audio_system.h"

#include "media/audio/audio_system_impl.h"
#include "services/audio/public/cpp/audio_system_to_service_adapter.h"
#include "services/service_manager/public/cpp/connector.h"

namespace audio {

namespace audio_system_factory {

std::unique_ptr<media::AudioSystem> CreateInstance(
    std::unique_ptr<service_manager::Connector> connector) {
  const bool kSystemInfoViaService = true;
  if (kSystemInfoViaService) {
    return std::make_unique<audio::AudioSystemToServiceAdapter>(
        std::move(connector));
  }
  return media::AudioSystemImpl::CreateInstance();
}

}  // namespace audio_system_factory

}  // namespace audio
