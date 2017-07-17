// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CredentialManagerTraits_h
#define CredentialManagerTraits_h

#include "mojo/public/cpp/bindings/struct_traits.h"
#include "mojo/public/cpp/bindings/string_traits_wtf.h"
#include "modules/credentialmanager/PasswordCredential.h"
#include "public/platform/modules/credentialmanager/v2.mojom-blink.h"
#include "platform/wtf/text/WTFString.h"

namespace mojo {

template <>
struct StructTraits<credential_manager::mojom::blink::PasswordCredential::DataView,
                    ::blink::PasswordCredential*> {
  static WTF::String id(::blink::PasswordCredential*);
  static WTF::String password(::blink::PasswordCredential*);

  static bool Read(credential_manager::mojom::blink::PasswordCredential::DataView,
                   ::blink::PasswordCredential** out);
};

}  // namespace mojo

#endif  // CredentialManagerTraits_h
