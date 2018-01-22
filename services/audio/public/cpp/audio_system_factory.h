// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_PUBLIC_CPP_AUDIO_SYSTEM_FACTORY_H_
#define SERVICES_AUDIO_PUBLIC_CPP_AUDIO_SYSTEM_FACTORY_H_

#include <memory>

#include "media/audio/audio_system.h"

namespace service_manager {
class Connector;
}

namespace audio {

namespace audio_system_factory {

// Will clone the connector.
std::unique_ptr<media::AudioSystem> CreateInstance(
    service_manager::Connector* connector);

}  // namespace audio_system_factory

}  // namespace audio

#endif  // SERVICES_AUDIO_PUBLIC_CPP_AUDIO_SYSTEM_FACTORY_H_
