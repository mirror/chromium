// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ASSISTANT_ASSISTANT_SERVICE_H_
#define CHROME_BROWSER_CHROMEOS_ASSISTANT_ASSISTANT_SERVICE_H_

#include <memory>
#include <string>

// TODO(xiaohuic): replace with "base/macros.h" once we remove
// libassistant/contrib dependency.
#include "chrome/browser/chromeos/assistant/platform_api_impl.h"
#include "libassistant/contrib/core/macros.h"

namespace assistant_client {
class AssistantManager;
class AssistantManagerInternal;
class PlatformApi;
}  // namespace assistant_client

namespace chromeos {
namespace assistant {

class AssistantServiceManager {
 public:
  class AuthTokenProvider {
   public:
    virtual std::string& GetAccessToken();

   protected:
    virtual ~AuthTokenProvider();
  };

  explicit AssistantServiceManager(AuthTokenProvider* auth_token_provider);
  virtual ~AssistantServiceManager();

  void Start();

 private:
  std::unique_ptr<PlatformApiImpl> platform_api_;
  std::unique_ptr<assistant_client::AssistantManager> assistant_manager_;
  assistant_client::AssistantManagerInternal* assistant_manager_internal_;
  AuthTokenProvider* auth_token_provider_;

  DISALLOW_COPY_AND_ASSIGN(AssistantServiceManager);
};

}  // namespace assistant
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_ASSISTANT_ASSISTANT_SERVICE_H_
