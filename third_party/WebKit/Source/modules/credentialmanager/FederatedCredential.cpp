// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/credentialmanager/FederatedCredential.h"

#include "bindings/core/v8/ExceptionState.h"
#include "modules/credentialmanager/FederatedCredentialInit.h"
#include "platform/credentialmanager/PlatformFederatedCredential.h"
#include "platform/weborigin/SecurityOrigin.h"

namespace blink {

FederatedCredential* FederatedCredential::Create(
    const String& id,
    RefPtr<SecurityOrigin> provider,
    const String& name,
    const KURL& icon) {
  return new FederatedCredential(id, provider, name, icon);
}

FederatedCredential* FederatedCredential::Create(
    const FederatedCredentialInit& data,
    ExceptionState& exception_state) {
  if (data.id().IsEmpty()) {
    exception_state.ThrowTypeError("'id' must not be empty.");
    return nullptr;
  }
  if (data.provider().IsEmpty()) {
    exception_state.ThrowTypeError("'provider' must not be empty.");
    return nullptr;
  }

  KURL icon_url = ParseStringAsURL(data.iconURL(), exception_state);
  KURL provider_url = ParseStringAsURL(data.provider(), exception_state);
  if (exception_state.HadException())
    return nullptr;
  return new FederatedCredential(
      data.id(), SecurityOrigin::Create(provider_url), data.name(), icon_url);
}

FederatedCredential::FederatedCredential(const String& id,
                                         RefPtr<SecurityOrigin> provider,
                                         const String& name,
                                         const KURL& icon)
    : Credential(
          PlatformFederatedCredential::Create(id, provider, name, icon)) {}

const String FederatedCredential::provider() const {
  return static_cast<PlatformFederatedCredential*>(platform_credential_.Get())
      ->Provider()
      ->ToString();
}

const String& FederatedCredential::name() const {
  return static_cast<PlatformFederatedCredential*>(platform_credential_.Get())
      ->Name();
}

const KURL& FederatedCredential::iconURL() const {
  return static_cast<PlatformFederatedCredential*>(platform_credential_.Get())
      ->IconURL();
}

}  // namespace blink
