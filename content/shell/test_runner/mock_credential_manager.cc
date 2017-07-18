// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/test_runner/mock_credential_manager.h"

#include <memory>
#include <utility>

#include "base/strings/utf_string_conversions.h"

namespace test_runner {

MockCredentialManager::MockCredentialManager()
    : error_(password_manager::mojom::CredentialManagerError::SUCCESS) {}

MockCredentialManager::~MockCredentialManager() {}

void MockCredentialManager::BindRequest(
    password_manager::mojom::CredentialManagerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void MockCredentialManager::SetResponse(const std::string& id,
                                        const std::string& name,
                                        const std::string& avatar,
                                        const std::string& password) {
  credential_.type = password_manager::CredentialType::CREDENTIAL_TYPE_PASSWORD;
  credential_.id = base::UTF8ToUTF16(id);
  credential_.name = base::UTF8ToUTF16(name);
  credential_.icon = GURL(avatar);
  credential_.password = base::UTF8ToUTF16(password);
}

void MockCredentialManager::ClearResponse() {
  credential_ = password_manager::CredentialInfo();
}

void MockCredentialManager::SetError(const std::string& error) {
  if (error == "pending")
    error_ = password_manager::mojom::CredentialManagerError::PENDINGREQUEST;
  if (error == "disabled")
    error_ = password_manager::mojom::CredentialManagerError::DISABLED;
  if (error == "unknown")
    error_ = password_manager::mojom::CredentialManagerError::UNKNOWN;
  if (error.empty())
    error_ = password_manager::mojom::CredentialManagerError::SUCCESS;
}

void MockCredentialManager::Store(
    const password_manager::CredentialInfo& credential,
    StoreCallback callback) {
  std::move(callback).Run();
}

void MockCredentialManager::PreventSilentAccess(
    PreventSilentAccessCallback callback) {
  std::move(callback).Run();
}

void MockCredentialManager::Get(
    password_manager::CredentialMediationRequirement mediation,
    bool include_passwords,
    const std::vector<GURL>& federations,
    GetCallback callback) {
  if (error_ != password_manager::mojom::CredentialManagerError::SUCCESS) {
    std::move(callback).Run(error_, password_manager::CredentialInfo());
  } else {
    std::move(callback).Run(error_, credential_);
    credential_ = password_manager::CredentialInfo();
  }
}

}  // namespace test_runner
