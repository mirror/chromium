// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_ASSISTANT_ASSISTANT_MANAGER_SERVICE_IMPL_H_
#define SERVICES_ASSISTANT_ASSISTANT_MANAGER_SERVICE_IMPL_H_

#include <memory>
#include <string>

// TODO(xiaohuic): replace with "base/macros.h" once we remove
// libassistant/contrib dependency.
#include "libassistant/contrib/core/macros.h"
#include "services/assistant/assistant_manager_service.h"
#include "services/assistant/platform_api_impl.h"

namespace assistant_client {
class AssistantManager;
class AssistantManagerInternal;
}  // namespace assistant_client

namespace assistant {

class AssistantManagerServiceImpl : public AssistantManagerService {
 public:
  explicit AssistantManagerServiceImpl(
      AssistantManagerService::AuthTokenProvider* auth_token_provider);
  ~AssistantManagerServiceImpl() override;

  // assistant::AssistantManagerService overrides
  void Start() override;

 private:
  AuthTokenProvider* const auth_token_provider_;
  std::unique_ptr<PlatformApiImpl> platform_api_;
  std::unique_ptr<assistant_client::AssistantManager> assistant_manager_;
  assistant_client::AssistantManagerInternal* const assistant_manager_internal_;

  DISALLOW_COPY_AND_ASSIGN(AssistantManagerServiceImpl);
};

}  // namespace assistant

#endif  // SERVICES_ASSISTANT_ASSISTANT_MANAGER_SERVICE_IMPL_H_
