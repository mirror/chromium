// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/passwords/js_credential_manager.h"

#import <Foundation/Foundation.h>

#include "base/json/string_escape.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace credential_manager {

namespace {

// Returns quoted string as NSString*. It escapes quotes in |str| so that it's
// safe to pass the string to JavaScript.
NSString* GetQuotedNSString(base::string16 str) {
  return base::SysUTF8ToNSString(base::GetQuotedJSONString(str));
}

// Returns quoted string as NSString*. It escapes quotes in |str| so that it's
// safe to pass the string to JavaScript.
NSString* GetQuotedNSString(std::string str) {
  return base::SysUTF8ToNSString(base::GetQuotedJSONString(str));
}

// Takes CredentianInfo and returns NSString* representing invocation of
// Credential's constructor with proper values and type. This function must be
// used with CredentialInfo obtained from PasswordStore because it doesn't
// validate its fields.
NSString* CredentialInfoToJsCredential(password_manager::CredentialInfo info) {
  if (info.type ==
      password_manager::CredentialType::CREDENTIAL_TYPE_FEDERATED) {
    return
        [NSString stringWithFormat:
                      @"new FederatedCredential({id: %@, name: %@, iconURL: "
                      @"%@, provider: %@})",
                      GetQuotedNSString(info.id), GetQuotedNSString(info.name),
                      GetQuotedNSString(info.icon.spec()),
                      GetQuotedNSString(info.federation.GetURL().spec())];
  }
  if (info.type == password_manager::CredentialType::CREDENTIAL_TYPE_PASSWORD) {
    return
        [NSString stringWithFormat:
                      @"new PasswordCredential({id: %@, name: %@, iconURL: %@, "
                      @"password: %@})",
                      GetQuotedNSString(info.id), GetQuotedNSString(info.name),
                      GetQuotedNSString(info.icon.spec()),
                      GetQuotedNSString(info.password)];
  }
  NOTREACHED() << "Invalid CredentialType";
  return @"";
}

}  // namespace

JsCredentialManager::JsCredentialManager(web::WebState* web_state)
    : web_state_(web_state) {}

void JsCredentialManager::ResolvePromiseWithCredentialInfo(
    int request_id,
    base::Optional<password_manager::CredentialInfo> info) {
  if (!web_state_) {
    return;
  }
  NSString* credential_str =
      info.has_value() ? CredentialInfoToJsCredential(info.value()) : @"";
  NSString* script = [NSString
      stringWithFormat:@"__gCrWeb.credentialManager.resolvers_[%d](%@);",
                       request_id, credential_str];
  web_state_->ExecuteJavaScript(base::SysNSStringToUTF16(script));
}

void JsCredentialManager::ResolvePromiseWithVoid(int request_id) {
  if (!web_state_) {
    return;
  }
  NSString* script = [NSString
      stringWithFormat:@"__gCrWeb.credentialManager.resolvers_[%d]();",
                       request_id];
  web_state_->ExecuteJavaScript(base::SysNSStringToUTF16(script));
}

void JsCredentialManager::RejectPromiseWithTypeError(int request_id,
                                                     base::string16 message) {
  if (!web_state_) {
    return;
  }
  NSString* script = [NSString
      stringWithFormat:
          @"__gCrWeb.credentialManager.rejecters_[%d](new TypeError(%@));",
          request_id, GetQuotedNSString(message)];
  web_state_->ExecuteJavaScript(base::SysNSStringToUTF16(script));
}

void JsCredentialManager::RejectPromiseWithInvalidStateError(
    int request_id,
    base::string16 message) {
  if (!web_state_) {
    return;
  }
  NSString* script =
      [NSString stringWithFormat:
                    @"__gCrWeb.credentialManager.rejecters_[%d](new "
                    @"DOMException(%@, DOMException.INVALID_STATE_ERR));",
                    request_id, GetQuotedNSString(message)];
  web_state_->ExecuteJavaScript(base::SysNSStringToUTF16(script));
}

void JsCredentialManager::RejectPromiseWithNotSupportedError(
    int request_id,
    base::string16 message) {
  if (!web_state_) {
    return;
  }
  NSString* script =
      [NSString stringWithFormat:
                    @"__gCrWeb.credentialManager.rejecters_[%d](new "
                    @"DOMException(%@, DOMException.NOT_SUPPORTED_ERR));",
                    request_id, GetQuotedNSString(message)];
  web_state_->ExecuteJavaScript(base::SysNSStringToUTF16(script));
}

}  // namespace credential_manager
