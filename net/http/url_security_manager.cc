// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/url_security_manager.h"

#include <utility>

#include "net/http/http_auth_filter.h"

namespace net {

URLSecurityManager::URLSecurityManager() = default;

URLSecurityManager::~URLSecurityManager() = default;

URLSecurityManagerWhitelist::URLSecurityManagerWhitelist() = default;

URLSecurityManagerWhitelist::~URLSecurityManagerWhitelist() = default;

bool URLSecurityManagerWhitelist::CanUseExplicitCredentialsForNTLM(
    const GURL& auth_origin) const {
  return ntlm_credentials_policy_ != DISALLOW_NTLM;
}

bool URLSecurityManagerWhitelist::CanUseAmbientCredentialsForNTLM(
    const GURL& auth_origin) const {
  return ntlm_credentials_policy_ == ALLOW_AMBIENT_CREDENTIALS_WITH_NTLM &&
         CanUseAmbientCredentialsForNegotiate(auth_origin);
}

bool URLSecurityManagerWhitelist::CanUseAmbientCredentialsForNegotiate(
    const GURL& auth_origin) const {
  return whitelist_default_.get() &&
         whitelist_default_->IsValid(auth_origin, HttpAuth::AUTH_SERVER);
}

bool URLSecurityManagerWhitelist::CanDelegate(const GURL& auth_origin) const {
  return whitelist_delegate_.get() &&
         whitelist_delegate_->IsValid(auth_origin, HttpAuth::AUTH_SERVER);
}

void URLSecurityManagerWhitelist::SetDefaultWhitelist(
    std::unique_ptr<HttpAuthFilter> whitelist_default) {
  whitelist_default_ = std::move(whitelist_default);
}

void URLSecurityManagerWhitelist::SetDelegateWhitelist(
    std::unique_ptr<HttpAuthFilter> whitelist_delegate) {
  whitelist_delegate_ = std::move(whitelist_delegate);
}

bool URLSecurityManagerWhitelist::HasDefaultWhitelist() const {
  return whitelist_default_.get() != nullptr;
}

}  //  namespace net
