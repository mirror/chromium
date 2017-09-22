// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_AUDIO_SERVICE_H_
#define SERVICES_AUDIO_AUDIO_SERVICE_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "services/audio/public/interfaces/audio_system_info.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace media {
class AudioManager;
}

namespace audio {
class AudioSystemInfo;

class AudioService : public service_manager::Service {
 public:
  explicit AudioService(media::AudioManager* audio_manager_);
  ~AudioService() final;

  // service_manager::Service implementation.
  void OnStart() final;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) final;
  bool OnServiceManagerConnectionLost() final;

 private:
  void MaybeRequestQuitDelayed();
  void MaybeRequestQuit();

  void BindAudioSystemInfoRequest(mojom::AudioSystemInfoRequest request);

  media::AudioManager* GetAudioManager();

  std::unique_ptr<service_manager::ServiceContextRefFactory> ref_factory_;
  service_manager::BinderRegistry registry_;

  std::unique_ptr<media::AudioManager> own_audio_manager_;
  media::AudioManager* audio_manager_;

  std::unique_ptr<AudioSystemInfo> audio_system_info_;

  base::WeakPtrFactory<AudioService> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(AudioService);
};

}  // namespace audio
#endif  // SERVICES_AUDIO_AUDIO_SERVICE_H_
