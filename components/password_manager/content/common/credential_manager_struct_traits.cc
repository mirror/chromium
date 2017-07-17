// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/content/common/credential_manager_struct_traits.h"

#include "url/mojo/origin_struct_traits.h"
#include "url/mojo/url_gurl_struct_traits.h"

using namespace credential_manager;

namespace mojo {

// static
mojom::CredentialMediationRequirement
EnumTraits<mojom::CredentialMediationRequirement,
           password_manager::CredentialMediationRequirement>::
    ToMojom(password_manager::CredentialMediationRequirement input) {
  switch (input) {
    case password_manager::CredentialMediationRequirement::kSilent:
      return mojom::CredentialMediationRequirement::kSilent;
    case password_manager::CredentialMediationRequirement::kOptional:
      return mojom::CredentialMediationRequirement::kOptional;
    case password_manager::CredentialMediationRequirement::kRequired:
      return mojom::CredentialMediationRequirement::kRequired;
  }

  NOTREACHED();
  return mojom::CredentialMediationRequirement::kOptional;
}

// static
bool EnumTraits<mojom::CredentialMediationRequirement,
                password_manager::CredentialMediationRequirement>::
    FromMojom(mojom::CredentialMediationRequirement input,
              password_manager::CredentialMediationRequirement* output) {
  switch (input) {
    case mojom::CredentialMediationRequirement::kSilent:
      *output = password_manager::CredentialMediationRequirement::kSilent;
      return true;
    case mojom::CredentialMediationRequirement::kOptional:
      *output = password_manager::CredentialMediationRequirement::kOptional;
      return true;
    case mojom::CredentialMediationRequirement::kRequired:
      *output = password_manager::CredentialMediationRequirement::kRequired;
      return true;
  }

  NOTREACHED();
  return false;
}

// static
credential_manager::mojom::Credential::Tag
UnionTraits<credential_manager::mojom::CredentialDataView,
            password_manager::CredentialInfo>::
    GetTag(const password_manager::CredentialInfo& info) {
  switch (info.type) {
    case password_manager::CredentialType::CREDENTIAL_TYPE_PASSWORD:
      return credential_manager::mojom::Credential::Tag::PASSWORD_CREDENTIAL;
    case password_manager::CredentialType::CREDENTIAL_TYPE_FEDERATED:
      return credential_manager::mojom::Credential::Tag::FEDERATED_CREDENTIAL;
    case password_manager::CredentialType::CREDENTIAL_TYPE_EMPTY:
      break;
  }
  NOTREACHED();
  return static_cast<credential_manager::mojom::Credential::Tag>(-1);
}

// static
bool UnionTraits<mojom::CredentialDataView, password_manager::CredentialInfo>::
    Read(mojom::CredentialDataView data,
         password_manager::CredentialInfo* out) {
  if (data.is_password_credential())
    return data.ReadPasswordCredential(out);
  if (data.is_federated_credential())
    return data.ReadFederatedCredential(out);

  NOTREACHED();
  return false;
}

// static
bool StructTraits<mojom::PasswordCredentialDataView,
                  password_manager::CredentialInfo>::
    Read(mojom::PasswordCredentialDataView data,
         password_manager::CredentialInfo* out) {
  out->type = password_manager::CredentialType::CREDENTIAL_TYPE_PASSWORD;
  return data.ReadId(&out->id) && data.ReadName(&out->name) &&
         data.ReadIcon(&out->icon) && data.ReadPassword(&out->password);
}

// static
bool StructTraits<mojom::FederatedCredentialDataView,
                  password_manager::CredentialInfo>::
    Read(mojom::FederatedCredentialDataView data,
         password_manager::CredentialInfo* out) {
  out->type = password_manager::CredentialType::CREDENTIAL_TYPE_FEDERATED;
  return data.ReadId(&out->id) && data.ReadName(&out->name) &&
         data.ReadIcon(&out->icon) && data.ReadFederation(&out->federation);
}

}  // namespace mojo
