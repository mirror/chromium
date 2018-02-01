// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_ASSISTANT_PLATFORM_API_IMPL_H_
#define SERVICES_ASSISTANT_PLATFORM_API_IMPL_H_

#include <memory>
#include <string>

// TODO(xiaohuic): replace with "base/macros.h" once we remove
// libassistant/contrib dependency.
#include "libassistant/contrib/core/macros.h"
#include "libassistant/shared/public/platform_api.h"

namespace assistant_client {
class AudioInputProvider;
class AudioOutputProvider;
class AuthProvider;
class FileProvider;
class NetworkProvider;
class ResourceProvider;
class SystemProvider;
}  // namespace assistant_client

namespace assistant_contrib {
class AudioInputProviderImpl;
class AudioOutputProviderImpl;
class FileProviderImpl;
class NetworkProviderImpl;
class ResourceProviderImpl;
class SystemProviderImpl;
}  // namespace assistant_contrib

namespace assistant {

// Platform API required by the voice assistant.
class PlatformApiImpl : public assistant_client::PlatformApi {
 public:
  explicit PlatformApiImpl(const std::string& config);
  ~PlatformApiImpl() override;

  // assistant_client::PlatformApi overrides
  assistant_client::AudioInputProvider& GetAudioInputProvider() override;
  assistant_client::AudioOutputProvider& GetAudioOutputProvider() override;
  assistant_client::AuthProvider& GetAuthProvider() override;
  assistant_client::FileProvider& GetFileProvider() override;
  assistant_client::NetworkProvider& GetNetworkProvider() override;
  assistant_client::ResourceProvider& GetResourceProvider() override;
  assistant_client::SystemProvider& GetSystemProvider() override;

 private:
  class DummyAuthProvider;

  std::unique_ptr<assistant_contrib::AudioInputProviderImpl>
      audio_input_provider_;
  std::unique_ptr<assistant_contrib::AudioOutputProviderImpl>
      audio_output_provider_;
  std::unique_ptr<DummyAuthProvider> auth_provider_;
  std::unique_ptr<assistant_contrib::FileProviderImpl> file_provider_;
  std::unique_ptr<assistant_contrib::NetworkProviderImpl> network_provider_;
  std::unique_ptr<assistant_contrib::ResourceProviderImpl> resource_provider_;
  std::unique_ptr<assistant_contrib::SystemProviderImpl> system_provider;

  DISALLOW_COPY_AND_ASSIGN(PlatformApiImpl);
};

}  // namespace assistant

#endif  // SERVICES_ASSISTANT_PLATFORM_API_IMPL_H_
