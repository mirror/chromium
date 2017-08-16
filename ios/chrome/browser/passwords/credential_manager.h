// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef IOS_CHROME_BROWSER_PASSWORDS_CREDENTIAL_MANAGER_H_
#define IOS_CHROME_BROWSER_PASSWORDS_CREDENTIAL_MANAGER_H_

#include "components/password_manager/core/browser/credential_manager_impl.h"
#import "ios/web/public/web_state/web_state.h"

namespace password_manager {

class PasswordManagerClient;

// Subclass of CredentialManagerImpl, owned by PasswordController. It is
// responsible for registering and handling callbacks for JS methods
// |navigator.credentials.*|.
// TODO(crbug.com/435047): Implement JSCredentialManager responsible for
// sending results back to website. Expected flow of CredentialManager class:
// 0. Add script command callbacks, initialize JSCredentialManager
// 1. JS message is sent
// 2. HandleScriptCommand is called, it parses the message and constructs a
//     OnceCallback to be passed as parameter to proper CredentialManagerImpl
//     method. |requestId| field from received JS message is bound to
//     constructed OnceCallback.
// 3. CredentialManagerImpl method is invoked, performs some logic with
//     PasswordStore, calls passed OnceCallback with result.
// 4. The OnceCallback uses JSCredentialManager to send result back to the
//     website.
class CredentialManager : public CredentialManagerImpl {
 public:
  explicit CredentialManager(PasswordManagerClient* client,
                             web::WebState* webState);

  // HandleScriptCommand parses JSON message and invokes Get, Store or
  // PreventSilentAccess on CredentialManagerImpl.
  bool HandleScriptCommand(const base::DictionaryValue& JSON,
                           const GURL& originURL,
                           bool userIsInteracting);

  // Exposed publicly for testing.
  static CredentialMediationRequirement ParseMediationRequirement(
      const base::DictionaryValue& JSON);
  static bool ParseIncludePasswords(const base::DictionaryValue& JSON);
  static std::vector<GURL> ParseFederations(const base::DictionaryValue& JSON);

 private:
  static CredentialType ParseCredentialType(const std::string& str);
  static CredentialInfo ParseCredentialDictionary(
      const base::DictionaryValue& JSON);

  // Passed as callback to CredentialManagerImpl::Get.
  void SendGetResponse(int requestId,
                       CredentialManagerError error,
                       const base::Optional<CredentialInfo>& info);
  // Passed as callback to CredentialManagerImpl::PreventSilentAccess.
  void SendPreventSilentAccessResponse(int requestId);
  // Passed as callback to CredentialManagerImpl::Store.
  void SendStoreResponse(int requestId);

  DISALLOW_COPY_AND_ASSIGN(CredentialManager);
};

};  // namespace password_manager

#endif  // IOS_CHROME_BROWSER_PASSWORDS_CREDENTIAL_MANAGER_H_
