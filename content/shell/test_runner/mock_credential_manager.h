// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_TEST_RUNNER_MOCK_CREDENTIAL_MANAGER_H_
#define CONTENT_SHELL_TEST_RUNNER_MOCK_CREDENTIAL_MANAGER_H_

#include <memory>

#include "base/macros.h"
#include "content/shell/test_runner/test_runner_export.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "third_party/WebKit/public/platform/WebCredentialManagerError.h"
#include "third_party/WebKit/public/platform/WebCredentialMediationRequirement.h"
#include "third_party/WebKit/public/platform/WebVector.h"
#include "third_party/WebKit/public/platform/modules/credentialmanager/credential_manager_v2.mojom.h"

namespace test_runner {

class TEST_RUNNER_EXPORT MockCredentialManager
    : public NON_EXPORTED_BASE(credential_manager::mojom::CredentialManager) {
 public:
  MockCredentialManager();
  ~MockCredentialManager() override;

  void BindRequest(credential_manager::mojom::CredentialManagerRequest request);

  void SetResponse(const std::string& id,
                   const std::string& name,
                   const std::string& avatar,
                   const std::string& password);
  void ClearResponse();
  void SetError(const std::string& error);

  // credential_manager::mojom::CredentialManager:
  void Store(const password_manager::CredentialInfo& credential,
             StoreCallback callback) override;
  void PreventSilentAccess(PreventSilentAccessCallback callback) override;
  void Get(password_manager::CredentialMediationRequirement mediation,
           bool include_passwords,
           const std::vector<GURL>& federations,
           GetCallback callback) override;

 private:
  mojo::BindingSet<credential_manager::mojom::CredentialManager> bindings_;

  password_manager::CredentialInfo credential_;
  credential_manager::mojom::CredentialManagerError error_;

  DISALLOW_COPY_AND_ASSIGN(MockCredentialManager);
};

}  // namespace test_runner

#endif  // CONTENT_SHELL_TEST_RUNNER_MOCK_CREDENTIAL_MANAGER_H_
