// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CONTENT_COMMON_CREDENTIAL_MANAGER_STRUCT_TRAITS_H_
#define COMPONENTS_PASSWORD_MANAGER_CONTENT_COMMON_CREDENTIAL_MANAGER_STRUCT_TRAITS_H_

#include "base/strings/string16.h"
#include "components/password_manager/core/common/credential_manager_types.h"
#include "mojo/public/cpp/bindings/enum_traits.h"
#include "mojo/public/cpp/bindings/struct_traits.h"
#include "mojo/public/cpp/bindings/union_traits.h"
#include "third_party/WebKit/public/platform/modules/credentialmanager/credential_manager.mojom.h"

namespace mojo {

template <>
struct EnumTraits<password_manager::mojom::CredentialMediationRequirement,
                  password_manager::CredentialMediationRequirement> {
  static password_manager::mojom::CredentialMediationRequirement ToMojom(
      password_manager::CredentialMediationRequirement input);
  static bool FromMojom(
      password_manager::mojom::CredentialMediationRequirement input,
      password_manager::CredentialMediationRequirement* output);
};

template <>
struct UnionTraits<password_manager::mojom::CredentialDataView,
                   password_manager::CredentialInfo> {
  static bool IsNull(const password_manager::CredentialInfo& r) {
    return r.type == password_manager::CredentialType::CREDENTIAL_TYPE_EMPTY;
  }

  static void SetToNull(password_manager::CredentialInfo* r) {
    r->type = password_manager::CredentialType::CREDENTIAL_TYPE_EMPTY;
  }

  static password_manager::mojom::Credential::Tag GetTag(
      const password_manager::CredentialInfo& r);

  static const password_manager::CredentialInfo& password_credential(
      const password_manager::CredentialInfo& r) {
    return r;
  }

  static password_manager::CredentialInfo federated_credential(
      const password_manager::CredentialInfo& r) {
    return r;
  }

  static bool Read(password_manager::mojom::CredentialDataView data,
                   password_manager::CredentialInfo* out);
};

template <>
struct StructTraits<password_manager::mojom::PasswordCredentialDataView,
                    password_manager::CredentialInfo> {
  static const base::string16& id(const password_manager::CredentialInfo& r) {
    return r.id;
  }

  static const base::string16& name(const password_manager::CredentialInfo& r) {
    return r.name;
  }

  static const GURL& icon(const password_manager::CredentialInfo& r) {
    return r.icon;
  }

  static const base::string16& password(
      const password_manager::CredentialInfo& r) {
    return r.password;
  }

  static bool Read(password_manager::mojom::PasswordCredentialDataView data,
                   password_manager::CredentialInfo* out);
};

template <>
struct StructTraits<password_manager::mojom::FederatedCredentialDataView,
                    password_manager::CredentialInfo> {
  static const base::string16& id(const password_manager::CredentialInfo& r) {
    return r.id;
  }

  static const base::string16& name(const password_manager::CredentialInfo& r) {
    return r.name;
  }

  static const GURL& icon(const password_manager::CredentialInfo& r) {
    return r.icon;
  }

  static const url::Origin& federation(
      const password_manager::CredentialInfo& r) {
    return r.federation;
  }

  static bool Read(password_manager::mojom::FederatedCredentialDataView data,
                   password_manager::CredentialInfo* out);
};

}  // namespace mojo

#endif  // COMPONENTS_PASSWORD_MANAGER_CONTENT_COMMON_CREDENTIAL_MANAGER_STRUCT_TRAITS_H_
