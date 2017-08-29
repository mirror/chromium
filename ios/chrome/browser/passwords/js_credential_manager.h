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

  void ResolvePromiseWithCredentialInfo(int request_id, base::Optional<password_manager::CredentialInfo> info);

  void ResolvePromiseWithVoid(int request_id);

  void RejectPromiseWithTypeError(int request_id, base::string16 message);

  void RejectPromiseWithInvalidStateError(int request_id, base::string16 message);

  void RejectPromiseWithNotSupportedError(int request_id, base::string16 message);

 private:
  web::WebState* web_state_;
};

}  // namespace credential_manager

#endif  // IOS_CHROME_BROWSER_PASSWORDS_JS_CREDENTIAL_MANAGER_H_
