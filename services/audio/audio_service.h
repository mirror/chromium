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
class AudioThread;
}  // namespace media

namespace audio {
class AudioSystemInfo;

class AudioService : public service_manager::Service {
 public:
  // Abstracts AudioManager ownership for AudioService: either holds a pointer
  // to an existing AudioManager, or creates one using a provided factory
  // callback abd manages its lifetime.
  // When AudioService runs in-process, non-owning version must be used to pass
  // an existing AudioManager.
  class AudioManagerAccessor {
   public:
    using AudioManagerFactory =
        base::OnceCallback<std::unique_ptr<media::AudioManager>(
            std::unique_ptr<media::AudioThread>)>;
    explicit AudioManagerAccessor(media::AudioManager* audio_manager);
    explicit AudioManagerAccessor(AudioManagerFactory audio_manager_factory);
    ~AudioManagerAccessor();

    // Returns task runner the service should run on if AudioManager is provided
    // externally, otherwise nullptr.
    base::SingleThreadTaskRunner* GetMandatoryTaskRunner();

    // Returns a pointer to AudioManager (creates one if required).
    media::AudioManager* GetAudioManager();

   private:
    base::Optional<AudioManagerFactory> audio_manager_factory_;
    std::unique_ptr<media::AudioManager> own_audio_manager_;
    media::AudioManager* audio_manager_ = nullptr;
    SEQUENCE_CHECKER(sequence_checker_);
    DISALLOW_COPY_AND_ASSIGN(AudioManagerAccessor);
  };

  explicit AudioService(
      std::unique_ptr<AudioManagerAccessor> audio_manager_accessor);
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

  std::unique_ptr<AudioManagerAccessor> audio_manager_accessor_;

  std::unique_ptr<AudioSystemInfo> audio_system_info_;

  std::unique_ptr<service_manager::ServiceContextRefFactory> ref_factory_;
  service_manager::BinderRegistry registry_;

  base::WeakPtrFactory<AudioService> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(AudioService);
};

}  // namespace audio
#endif  // SERVICES_AUDIO_AUDIO_SERVICE_H_
