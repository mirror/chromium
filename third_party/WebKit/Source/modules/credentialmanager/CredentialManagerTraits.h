// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CredentialManagerTraits_h
#define CredentialManagerTraits_h

#include "modules/credentialmanager/Credential.h"
#include "modules/credentialmanager/FederatedCredential.h"
#include "modules/credentialmanager/PasswordCredential.h"
#include "mojo/public/cpp/bindings/enum_traits.h"
#include "mojo/public/cpp/bindings/string_traits_wtf.h"
#include "mojo/public/cpp/bindings/struct_traits.h"
#include "mojo/public/cpp/bindings/union_traits.h"
#include "platform/heap/Handle.h"
#include "platform/weborigin/KURL.h"
#include "platform/wtf/RefPtr.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/WebCredentialManagerError.h"
#include "public/platform/WebCredentialMediationRequirement.h"
#include "public/platform/modules/credentialmanager/credential_manager.mojom-blink.h"
#include "third_party/WebKit/Source/platform/mojo/KURLStructTraits.h"
#include "third_party/WebKit/Source/platform/mojo/SecurityOriginStructTraits.h"

namespace mojo {

template <>
struct EnumTraits<
    password_manager::mojom::blink::CredentialMediationRequirement,
    blink::WebCredentialMediationRequirement> {
  static password_manager::mojom::blink::CredentialMediationRequirement ToMojom(
      blink::WebCredentialMediationRequirement input);
  static bool FromMojom(
      password_manager::mojom::blink::CredentialMediationRequirement input,
      blink::WebCredentialMediationRequirement* output);
};

template <>
struct EnumTraits<password_manager::mojom::blink::CredentialManagerError,
                  blink::WebCredentialManagerError> {
  static password_manager::mojom::blink::CredentialManagerError ToMojom(
      blink::WebCredentialManagerError input);
  static bool FromMojom(
      password_manager::mojom::blink::CredentialManagerError input,
      blink::WebCredentialManagerError* output);
};

template <>
struct UnionTraits<password_manager::mojom::blink::Credential::DataView,
                   blink::Persistent<blink::Credential>> {
  static bool IsNull(const blink::Persistent<blink::Credential>& info) {
    return !info;
  }

  static void SetToNull(blink::Persistent<blink::Credential>* out) {
    *out = nullptr;
  }

  static password_manager::mojom::blink::Credential::DataView::Tag GetTag(
      const blink::Persistent<blink::Credential>& input) {
    if (input->IsPassword()) {
      return password_manager::mojom::blink::Credential::Tag::
          PASSWORD_CREDENTIAL;
    } else {
      DCHECK(input->IsFederated());
      return password_manager::mojom::blink::Credential::Tag::
          FEDERATED_CREDENTIAL;
    }
  }

  static blink::Persistent<blink::PasswordCredential> password_credential(
      const blink::Persistent<blink::Credential>& input) {
    return static_cast<blink::PasswordCredential*>(input.Get());
  }

  static blink::Persistent<blink::FederatedCredential> federated_credential(
      const blink::Persistent<blink::Credential>& input) {
    return static_cast<blink::FederatedCredential*>(input.Get());
  }

  static bool Read(password_manager::mojom::blink::Credential::DataView,
                   blink::Persistent<blink::Credential>* out);
};

template <>
struct StructTraits<
    password_manager::mojom::blink::PasswordCredential::DataView,
    blink::Persistent<blink::PasswordCredential>> {
  static WTF::String id(const blink::Persistent<blink::PasswordCredential>& c) {
    return c->id();
  }

  static WTF::String name(
      const blink::Persistent<blink::PasswordCredential>& c) {
    return c->name();
  }

  static blink::KURL icon(
      const blink::Persistent<blink::PasswordCredential>& c) {
    return c->iconURL();
  }

  static WTF::String password(
      const blink::Persistent<blink::PasswordCredential>& c) {
    return c->password();
  }

  static bool Read(password_manager::mojom::blink::PasswordCredential::DataView,
                   blink::Persistent<blink::PasswordCredential>* out);
};

template <>
struct StructTraits<
    password_manager::mojom::blink::FederatedCredential::DataView,
    blink::Persistent<blink::FederatedCredential>> {
  static WTF::String id(
      const blink::Persistent<blink::FederatedCredential>& c) {
    return c->id();
  }

  static WTF::String name(
      const blink::Persistent<blink::FederatedCredential>& c) {
    return c->name();
  }

  static blink::KURL icon(
      const blink::Persistent<blink::FederatedCredential>& c) {
    return c->iconURL();
  }

  static RefPtr<blink::SecurityOrigin> federation(
      const blink::Persistent<blink::FederatedCredential>& c) {
    return blink::SecurityOrigin::CreateFromString(c->provider());
  }

  static bool Read(
      password_manager::mojom::blink::FederatedCredential::DataView,
      blink::Persistent<blink::FederatedCredential>* out);
};

}  // namespace mojo

#endif  // CredentialManagerTraits_h
