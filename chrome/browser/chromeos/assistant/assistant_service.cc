// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/assistant/assistant_service.h"

#include <memory>
#include <string>
#include <utility>

#include "base/logging.h"
#include "chrome/browser/chromeos/assistant/internal/internal_constants.h"
#include "chrome/browser/chromeos/assistant/internal/internal_util.h"
#include "chrome/browser/chromeos/assistant/platform_api_impl.h"

namespace chromeos {
namespace assistant {

AssistantService::AssistantService(AuthTokenProvider* auth_token_provider) {
  auth_token_provider_ = auth_token_provider;
  platform_api_ = std::make_unique<PlatformApiImpl>(kDefaultConfigStr);
  assistant_manager_.reset(assistant_client::AssistantManager::Create(
      platform_api_.get(), kDefaultConfigStr));
  assistant_manager_internal_ =
      UnwrapAssistantManagerInternal(assistant_manager_.get());
}

AssistantService::~AssistantService() = default;

void AssistantService::Start() {
  auto* internal_options =
      assistant_manager_internal_->CreateDefaultInternalOptions();
  SetAssistantOptions(internal_options);
  assistant_manager_internal_->SetOptions(*internal_options, [](bool success) {
    VLOG(2) << "set options: " << success;
  });

  // By default AssistantManager will not start listening for its hotword until
  // we explicitly set EnableListening() to |true|.
  assistant_manager_->EnableListening(true);

  // Push the |access_token| we got as an argument into AssistantManager before
  // starting to ensure that all server requests will be authenticated once
  // it is started. |user_id| is used to pair a user to their |access_token|,
  // since we do not support multi-user in this example we can set it to a
  // dummy value like "0".
  assistant_manager_->SetAuthTokens({std::pair<std::string, std::string>(
      /* user_id: */ "0", auth_token_provider_->GetAccessToken())});

  assistant_manager_->Start();
}

}  // namespace assistant
}  // namespace chromeos
