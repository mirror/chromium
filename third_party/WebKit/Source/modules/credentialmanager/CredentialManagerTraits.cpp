// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/credentialmanager/CredentialManagerTraits.h"

namespace mojo {

using CredentialTraits =
    StructTraits<credential_manager::mojom::blink::PasswordCredential::DataView,
                 ::blink::PasswordCredential*>;

// static
WTF::String CredentialTraits::id(::blink::PasswordCredential* r) {
  return g_empty_string;
}

// static
WTF::String CredentialTraits::password(::blink::PasswordCredential* r) {
  return g_empty_string;
}

// static
bool CredentialTraits::
    Read(credential_manager::mojom::blink::PasswordCredential::DataView data,
         ::blink::PasswordCredential** out) {
  WTF::String id, password;
  if (!data.ReadId(&id) || !data.ReadPassword(&password))
    return false;
  *out = ::blink::PasswordCredential::Create(id, password);
  return true;
}

}  // namespace mojo
