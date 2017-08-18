// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/passwords/credential_manager.h"

#import "base/mac/bind_objc_block.h"
#import "ios/web/public/origin_util.h"
#include "url/origin.h"

using password_manager::CredentialManagerError;
using password_manager::CredentialInfo;
using password_manager::CredentialType;
using password_manager::CredentialMediationRequirement;

CredentialManager::CredentialManager(
    password_manager::PasswordManagerClient* client)
    : impl_(client) {
  // TODO(crbug.com/435047): Register a script command callback for prefix
  // "credentials" and HandleScriptCommand method.
}

CredentialManager::~CredentialManager() {}

bool CredentialManager::HandleScriptCommand(const base::DictionaryValue& json,
                                            const GURL& origin_url,
                                            bool user_is_interacting) {
  // TODO(crbug.com/435047): Check if context is secure.
  std::string command;
  if (!json.GetString("command", &command)) {
    DLOG(ERROR) << "RECEIVED BAD json - NO VALID 'command' FIELD";
    return false;
  }

  int request_id;
  if (!json.GetInteger("requestId", &request_id)) {
    DLOG(ERROR) << "RECEIVED BAD json - NO VALID 'requestId' FIELD";
    return false;
  }

  if (command == "credentials.get") {
    CredentialMediationRequirement mediation;
    if (!ParseMediationRequirement(json, &mediation)) {
      // TODO(crbug.com/435047): Reject promise with a TypeError.
      return false;
    }
    bool include_passwords;
    if (!ParseIncludePasswords(json, &include_passwords)) {
      // TODO(crbug.com/435047): Reject promise with a TypeError.
      return false;
    }
    std::vector<GURL> federations;
    if (!ParseFederations(json, &federations)) {
      // TODO(crbug.com/435047): Reject promise with a TypeError.
      return false;
    }
    impl_.Get(mediation, include_passwords, federations,
              base::BindOnce(&CredentialManager::SendGetResponse,
                             base::Unretained(this), request_id));
    return true;
  }
  if (command == "credentials.store") {
    CredentialInfo credential;
    if (!ParseCredentialDictionary(json, &credential)) {
      // TODO(crbug.com/435047): Reject promise with a TypeError.
      return false;
    }
    impl_.Store(credential,
                base::BindOnce(&CredentialManager::SendStoreResponse,
                               base::Unretained(this), request_id));
    return true;
  }
  if (command == "credentials.preventSilentAccess") {
    impl_.PreventSilentAccess(
        base::BindOnce(&CredentialManager::SendPreventSilentAccessResponse,
                       base::Unretained(this), request_id));
    return true;
  }
  return false;
}

bool CredentialManager::ParseMediationRequirement(
    const base::DictionaryValue& json,
    CredentialMediationRequirement* mediation) {
  std::string mediation_str;
  if (!json.HasKey("mediation")) {
    *mediation = CredentialMediationRequirement::kOptional;
    return true;
  }
  if (!json.GetString("mediation", &mediation_str)) {
    // Dictionary contains |mediation| field but it is not a valid string.
    return false;
  }
  if (mediation_str == "silent") {
    *mediation = CredentialMediationRequirement::kSilent;
    return true;
  }
  if (mediation_str == "required") {
    *mediation = CredentialMediationRequirement::kRequired;
    return true;
  }
  if (mediation_str == "optional") {
    *mediation = CredentialMediationRequirement::kOptional;
    return true;
  }
  // |mediation| value is not in {"silent", "required", "optional"}.
  // Dictionary is invalid then.
  return false;
}

bool CredentialManager::ParseIncludePasswords(const base::DictionaryValue& json,
                                              bool* include_passwords) {
  if (!json.HasKey("password")) {
    *include_passwords = false;
    return true;
  }
  if (!json.GetBoolean("password", include_passwords)) {
    // Dictionary contains |password| field but it is not a boolean. That means
    // dictionary is invalid.
    return false;
  }
  return true;
}

bool CredentialManager::ParseFederations(const base::DictionaryValue& json,
                                         std::vector<GURL>* federations) {
  federations->clear();
  if (!json.HasKey("providers")) {
    // No |providers| list.
    return true;
  }
  const base::ListValue* lst = nullptr;
  if (!json.GetList("providers", &lst)) {
    // Dictionary has |providers| field but it is not a list. That means
    // dictionary is invalid.
    return false;
  }
  for (size_t i = 0; i < lst->GetSize(); i++) {
    std::string s;
    if (!lst->GetString(i, &s)) {
      // Element of |providers| is invalid string.
      return false;
    }
    GURL gurl(s);
    if (gurl.is_valid()) {
      // Skip the invalid URLs. See
      // https://w3c.github.io/webappsec-credential-management/#provider-identification
      federations->push_back(GURL(s));
    }
  }
  return true;
}

bool CredentialManager::ParseCredentialType(const base::DictionaryValue& json,
                                            CredentialType* credential_type) {
  std::string str;
  if (!json.GetString("type", &str)) {
    // Credential must contain |type|
    return false;
  }
  if (str == "PasswordCredential") {
    *credential_type = CredentialType::CREDENTIAL_TYPE_PASSWORD;
    return true;
  }
  if (str == "FederatedCredential") {
    *credential_type = CredentialType::CREDENTIAL_TYPE_FEDERATED;
    return true;
  }
  return false;
}

bool CredentialManager::ParseCredentialDictionary(
    const base::DictionaryValue& json,
    CredentialInfo* credential) {
  if (!json.GetString("id", &credential->id)) {
    // |id| is required.
    return false;
  }
  json.GetString("name", &credential->name);
  std::string iconURL;
  if (json.GetString("iconURL", &iconURL) && !iconURL.empty()) {
    credential->icon = GURL(iconURL);
    if (!credential->icon.is_valid() ||
        !web::IsOriginSecure(credential->icon)) {
      // |iconURL| is either not a valid URL or not a secure URL.
      return false;
    }
  }
  if (!ParseCredentialType(json, &credential->type)) {
    // Credential has invalid |type|
    return false;
  }
  if (credential->type == CredentialType::CREDENTIAL_TYPE_PASSWORD) {
    if (!json.GetString("password", &credential->password) ||
        credential->password.empty()) {
      // |password| field is required for PasswordCredential.
      return false;
    }
  }
  if (credential->type == CredentialType::CREDENTIAL_TYPE_FEDERATED) {
    std::string federation;
    json.GetString("provider", &federation);
    if (!GURL(federation).is_valid()) {
      // |provider| field must be a valid URL. See
      // https://w3c.github.io/webappsec-credential-management/#provider-identification
      return false;
    }
    credential->federation = url::Origin(GURL(federation));
  }
  return true;
}

void CredentialManager::SendGetResponse(
    int request_id,
    CredentialManagerError error,
    const base::Optional<CredentialInfo>& info) {}

void CredentialManager::SendPreventSilentAccessResponse(int request_id) {}

void CredentialManager::SendStoreResponse(int request_id) {}
