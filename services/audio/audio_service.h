// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_AUDIO_SERVICE_H_
#define SERVICES_AUDIO_AUDIO_SERVICE_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "services/audio/public/interfaces/audio_system_info.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace media {
class AudioManager;
}  // namespace media

namespace audio {
class AudioSystemInfo;

class AudioService : public service_manager::Service {
 public:
  // Abstracts AudioManager ownership. Lives and must be accessed on a thread
  // its created on, and that thread must be AudioManager main thread.
  class AudioManagerAccessor {
   public:
    virtual ~AudioManagerAccessor(){};

    // Returns a pointer to AudioManager.
    virtual media::AudioManager* GetAudioManager() = 0;
  };

  AudioService(std::unique_ptr<AudioManagerAccessor> audio_manager_accessor,
               base::TimeDelta quit_timeout);
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

  const base::TimeDelta quit_timeout_;

  std::unique_ptr<AudioManagerAccessor> audio_manager_accessor_;
  std::unique_ptr<AudioSystemInfo> audio_system_info_;

  std::unique_ptr<service_manager::ServiceContextRefFactory> ref_factory_;
  service_manager::BinderRegistry registry_;

  // Thread it runs on should be the same as the main thread of AudioManager
  // provided by AudioManagerAccessor.
  THREAD_CHECKER(thread_checker_);
  base::WeakPtrFactory<AudioService> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(AudioService);
};

}  // namespace audio
#endif  // SERVICES_AUDIO_AUDIO_SERVICE_H_
