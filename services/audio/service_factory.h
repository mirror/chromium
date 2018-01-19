// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_SERVICE_FACTORY_H_
#define SERVICES_AUDIO_SERVICE_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "services/service_manager/public/cpp/service.h"

namespace media {
class AudioManager;
}  // namespace media

namespace audio {

namespace service_factory {

std::unique_ptr<service_manager::Service> CreateInProcess(
    media::AudioManager* audio_manager);

}  // namespace service_factory

}  // namespace audio

#endif  // SERVICES_AUDIO_SERVICE_FACTORY_H_
