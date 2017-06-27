// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/credentialmanager/CredentialUserData.h"
#include "platform/credentialmanager/PlatformFederatedCredential.h"
#include "platform/credentialmanager/PlatformPasswordCredential.h"

namespace blink {

CredentialUserData::CredentialUserData(PlatformCredential* platform_credential)
    : Credential(platform_credential) {}

const String& CredentialUserData::name() const {
  if (platform_credential_->IsPassword()) {
    return static_cast<PlatformPasswordCredential*>(platform_credential_.Get())
        ->Name();
  } else {
    return static_cast<PlatformFederatedCredential*>(platform_credential_.Get())
        ->Name();
  }
}

const KURL& CredentialUserData::iconURL() const {
  if (platform_credential_->IsPassword()) {
    return static_cast<PlatformPasswordCredential*>(platform_credential_.Get())
        ->IconURL();
  } else {
    return static_cast<PlatformFederatedCredential*>(platform_credential_.Get())
        ->IconURL();
  }
}
}  // namespace blink
