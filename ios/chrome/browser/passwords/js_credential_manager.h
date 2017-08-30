// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_JS_CREDENTIAL_MANAGER_H_
#define IOS_CHROME_BROWSER_PASSWORDS_JS_CREDENTIAL_MANAGER_H_

#include "base/optional.h"
#include "components/password_manager/core/common/credential_manager_types.h"

namespace web {
class WebState;
}

namespace credential_manager {

class JsCredentialManager {
 public:
  JsCredentialManager(web::WebState* web_state);

  // Resolves Promise with either Credential or undefined. This method should be
  // called as a result of successfully performing CredentialManager::Get.
  void ResolvePromiseWithCredentialInfo(
      int request_id,
      base::Optional<password_manager::CredentialInfo> info);

  // Resolves Promise with no value. This method should be called to send
  // acknowledge response from CredentialManager::Store and
  // CredentialManager::PreventSilentAccess.
  void ResolvePromiseWithVoid(int request_id);

  // Rejects Promise with TypeError. This may be a result of failed parsing of
  // arguments passed to exposed API method.
  void RejectPromiseWithTypeError(int request_id, base::string16 message);

  // Rejects Promise with InvalidStateError. This should be used when credential
  // manager is disabled or there is a pending 'get' request.
  void RejectPromiseWithInvalidStateError(int request_id,
                                          base::string16 message);

  // Rejects Promise with NotSupportedError. This should be used when password
  // store is unavailable or an unknown error occurs.
  void RejectPromiseWithNotSupportedError(int request_id,
                                          base::string16 message);

 private:
  web::WebState* web_state_;
};

}  // namespace credential_manager

#endif  // IOS_CHROME_BROWSER_PASSWORDS_JS_CREDENTIAL_MANAGER_H_
