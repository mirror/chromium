// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CredentialManagerTraits_h
#define CredentialManagerTraits_h

#include "mojo/public/cpp/bindings/enum_traits.h"
#include "mojo/public/cpp/bindings/string_traits_wtf.h"
#include "mojo/public/cpp/bindings/struct_traits.h"
#include "platform/mojo/KURLStructTraits.h"
#include "platform/mojo/SecurityOriginStructTraits.h"
#include "public/platform/WebCredential.h"
#include "public/platform/WebCredentialMediationRequirement.h"
#include "public/platform/WebFederatedCredential.h"
#include "public/platform/WebPasswordCredential.h"
#include "public/platform/WebSecurityOrigin.h"
#include "public/platform/WebString.h"
#include "public/platform/WebURL.h"
#include "public/platform/modules/credentialmanager/credential_manager.mojom-blink.h"

namespace mojo {

template <>
struct EnumTraits<
    password_manager::mojom::blink::CredentialMediationRequirement,
    ::blink::WebCredentialMediationRequirement> {
  static password_manager::mojom::blink::CredentialMediationRequirement ToMojom(
      ::blink::WebCredentialMediationRequirement input);
  static bool FromMojom(
      password_manager::mojom::blink::CredentialMediationRequirement input,
      ::blink::WebCredentialMediationRequirement* output);
};

template <>
struct EnumTraits<password_manager::mojom::blink::CredentialManagerError,
                  ::blink::WebCredentialManagerError> {
  static password_manager::mojom::blink::CredentialManagerError ToMojom(
      ::blink::WebCredentialManagerError input);
  static bool FromMojom(
      password_manager::mojom::blink::CredentialManagerError input,
      ::blink::WebCredentialManagerError* output);
};

template <>
struct StructTraits<password_manager::mojom::blink::CredentialInfo::DataView,
                    std::unique_ptr<::blink::WebCredential>> {
  static password_manager::mojom::blink::CredentialType type(
      const std::unique_ptr<::blink::WebCredential>&);
  static WTF::String id(const std::unique_ptr<::blink::WebCredential>&);
  static WTF::String name(const std::unique_ptr<::blink::WebCredential>&);
  static ::blink::KURL icon(const std::unique_ptr<::blink::WebCredential>&);
  static WTF::String password(const std::unique_ptr<::blink::WebCredential>&);
  static RefPtr<::blink::SecurityOrigin> federation(
      const std::unique_ptr<::blink::WebCredential>&);

  static bool Read(password_manager::mojom::blink::CredentialInfo::DataView,
                   std::unique_ptr<::blink::WebCredential>* out);
};

}  // namespace mojo

#endif  // CredentialManagerTraits_h
