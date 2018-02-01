// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/assistant/platform_api_impl.h"

#include <string>
#include <utility>
#include <vector>

#include "libassistant/contrib/platform/audio/input/audio_input_provider_impl.h"
#include "libassistant/contrib/platform/audio/output/audio_output_provider_impl.h"
#include "libassistant/contrib/platform/auth/auth_provider_impl.h"
#include "libassistant/contrib/platform/file/file_provider_impl.h"
#include "libassistant/contrib/platform/net/network_provider_impl.h"
#include "libassistant/contrib/platform/resources/resource_provider.h"
#include "libassistant/contrib/platform/system/system_provider.h"
#include "libassistant/shared/public/assistant_export.h"
#include "libassistant/shared/public/platform_api.h"
#include "libassistant/shared/public/platform_factory.h"

using assistant_client::AudioInputProvider;
using assistant_client::AudioOutputProvider;
using assistant_client::AuthProvider;
using assistant_client::FileProvider;
using assistant_client::NetworkProvider;
using assistant_client::ResourceProvider;
using assistant_client::SystemProvider;
using assistant_client::PlatformApi;
using assistant_client::ResourceProvider;

namespace assistant {

////////////////////////////////////////////////////////////////////////////////
// DummyAuthProvider
////////////////////////////////////////////////////////////////////////////////

// ChromeOS does not use auth manager, so we don't yet need to implement a
// real auth provider.
class PlatformApiImpl::DummyAuthProvider
    : public assistant_client::AuthProvider {
 public:
  DummyAuthProvider() = default;
  ~DummyAuthProvider() override = default;

  // assistant_client::AuthProvider overrides
  std::string GetAuthClientId() override { return "kFakeClientId"; }

  std::vector<std::string> GetClientCertificateChain() override { return {}; }

  void CreateCredentialAttestationJwt(
      const std::string& authorization_code,
      const std::vector<std::pair<std::string, std::string>>& claims,
      CredentialCallback attestation_callback) override {
    attestation_callback(Error::SUCCESS, "", "");
  }

  void CreateRefreshAssertionJwt(
      const std::string& key_identifier,
      const std::vector<std::pair<std::string, std::string>>& claims,
      AssertionCallback assertion_callback) override {
    assertion_callback(Error::SUCCESS, "");
  }

  void CreateDeviceAttestationJwt(
      const std::vector<std::pair<std::string, std::string>>& claims,
      AssertionCallback attestation_callback) override {
    attestation_callback(Error::SUCCESS, "");
  }

  std::string GetAttestationCertFingerprint() override {
    return "kFakeAttestationCertFingerprint";
  }

  void RemoveCredentialKey(const std::string& key_identifier) override {}

  void Reset() override {}
};

////////////////////////////////////////////////////////////////////////////////
// PlatformApiImpl
////////////////////////////////////////////////////////////////////////////////

PlatformApiImpl::PlatformApiImpl(const std::string& config) {
  audio_input_provider_ =
      std::make_unique<assistant_contrib::AudioInputProviderImpl>(config);
  audio_output_provider_ =
      std::make_unique<assistant_contrib::AudioOutputProviderImpl>(config,
                                                                   this);
  auth_provider_ = std::make_unique<DummyAuthProvider>();
  file_provider_ =
      std::make_unique<assistant_contrib::FileProviderImpl>(config);
  network_provider_ =
      std::make_unique<assistant_contrib::NetworkProviderImpl>(config);
  resource_provider_ =
      std::make_unique<assistant_contrib::ResourceProviderImpl>(config);
  system_provider =
      std::make_unique<assistant_contrib::SystemProviderImpl>(config);
}

PlatformApiImpl::~PlatformApiImpl() = default;

AudioInputProvider& PlatformApiImpl::GetAudioInputProvider() {
  return *audio_input_provider_;
}

AudioOutputProvider& PlatformApiImpl::GetAudioOutputProvider() {
  return *audio_output_provider_;
}

AuthProvider& PlatformApiImpl::GetAuthProvider() {
  return *auth_provider_;
}

FileProvider& PlatformApiImpl::GetFileProvider() {
  return *file_provider_;
}

NetworkProvider& PlatformApiImpl::GetNetworkProvider() {
  return *network_provider_;
}

ResourceProvider& PlatformApiImpl::GetResourceProvider() {
  return *resource_provider_;
}

SystemProvider& PlatformApiImpl::GetSystemProvider() {
  return *system_provider;
}

}  // namespace assistant
