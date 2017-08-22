// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_CREDENTIAL_MANAGER_UTIL_H_
#define IOS_CHROME_BROWSER_PASSWORDS_CREDENTIAL_MANAGER_UTIL_H_

#include "base/values.h"
#include "components/password_manager/core/common/credential_manager_types.h"

namespace credential_manager {

// Common Credential fields
constexpr char kCredentialIdKey[] = "id";
constexpr char kCredentialTypeKey[] = "type";
constexpr char kCredentialNameKey[] = "name";
constexpr char kCredentialIconKey[] = "iconURL";

// PasswordCredential and FederatedCredential own property keys
constexpr char kPasswordCredentialPasswordKey[] = "password";
constexpr char kFederatedCredentialProviderKey[] = "provider";

// CredentialRequestOptions fields
constexpr char kCredentialRequestMediationKey[] = "mediation";
constexpr char kCredentialRequestPasswordKey[] = "password";
constexpr char kCredentialRequestProvidersKey[] = "providers";

// CredentialRequestOptions.mediation possible values
constexpr char kMediationRequirementSilent[] = "silent";
constexpr char kMediationRequirementRequired[] = "required";
constexpr char kMediationRequirementOptional[] = "optional";

// Strings denoting possible Credential.type values
constexpr char kCredentialTypePassword[] = "PasswordCredential";
constexpr char kCredentialTypeFederated[] = "FederatedCredential";

// Returns value of Parse* methods below is false if |json| is invalid, which
// means it is missing required fields, contains fields of wrong type or
// unexpected values. Otherwise return value is true.
// Parses |mediation| field of JavaScript object CredentialRequestOptions.
bool ParseMediationRequirement(
    const base::DictionaryValue& json,
    password_manager::CredentialMediationRequirement* mediation);
// Parses |password| field of JavaScript object CredentialRequestOptions.
bool ParseIncludePasswords(const base::DictionaryValue& json,
                           bool* include_passwords);
// Parses |providers| field of JavaScript object
// FederatedCredentialRequestOptions into list of GURLs.
bool ParseFederations(const base::DictionaryValue& json,
                      std::vector<GURL>* federations);
// Parses |type| field from JavaScript Credential object into CredentialType.
bool ParseCredentialType(const base::DictionaryValue& json,
                         password_manager::CredentialType* credential_type);
// Parses dictionary representing JavaScript Credential object into
// CredentialInfo.
bool ParseCredentialDictionary(const base::DictionaryValue& json,
                               password_manager::CredentialInfo* credential);

}  // namespace credential_manager

#endif  // IOS_CHROME_BROWSER_PASSWORDS_CREDENTIAL_MANAGER_UTIL_H_
