// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/content/common/credential_manager_struct_traits.h"

#include "url/mojo/origin_struct_traits.h"
#include "url/mojo/url_gurl_struct_traits.h"

using namespace password_manager;

namespace mojo {

// static
mojom::CredentialMediationRequirement EnumTraits<
    mojom::CredentialMediationRequirement,
    CredentialMediationRequirement>::ToMojom(CredentialMediationRequirement
                                                 input) {
  switch (input) {
    case CredentialMediationRequirement::kSilent:
      return mojom::CredentialMediationRequirement::kSilent;
    case CredentialMediationRequirement::kOptional:
      return mojom::CredentialMediationRequirement::kOptional;
    case CredentialMediationRequirement::kRequired:
      return mojom::CredentialMediationRequirement::kRequired;
  }

  NOTREACHED();
  return mojom::CredentialMediationRequirement::kOptional;
}

// static
bool EnumTraits<mojom::CredentialMediationRequirement,
                CredentialMediationRequirement>::
    FromMojom(mojom::CredentialMediationRequirement input,
              CredentialMediationRequirement* output) {
  switch (input) {
    case mojom::CredentialMediationRequirement::kSilent:
      *output = CredentialMediationRequirement::kSilent;
      return true;
    case mojom::CredentialMediationRequirement::kOptional:
      *output = CredentialMediationRequirement::kOptional;
      return true;
    case mojom::CredentialMediationRequirement::kRequired:
      *output = CredentialMediationRequirement::kRequired;
      return true;
  }

  NOTREACHED();
  return false;
}

// static
mojom::Credential::Tag
UnionTraits<mojom::CredentialDataView, CredentialInfo>::GetTag(
    const CredentialInfo& info) {
  switch (info.type) {
    case CredentialType::CREDENTIAL_TYPE_PASSWORD:
      return mojom::Credential::Tag::PASSWORD_CREDENTIAL;
    case CredentialType::CREDENTIAL_TYPE_FEDERATED:
      return mojom::Credential::Tag::FEDERATED_CREDENTIAL;
    case CredentialType::CREDENTIAL_TYPE_EMPTY:
      break;
  }
  NOTREACHED();
  return static_cast<mojom::Credential::Tag>(-1);
}

// static
bool UnionTraits<mojom::CredentialDataView, CredentialInfo>::Read(
    mojom::CredentialDataView data,
    CredentialInfo* out) {
  if (data.is_password_credential())
    return data.ReadPasswordCredential(out);
  if (data.is_federated_credential())
    return data.ReadFederatedCredential(out);

  NOTREACHED();
  return false;
}

// static
bool StructTraits<mojom::PasswordCredentialDataView, CredentialInfo>::Read(
    mojom::PasswordCredentialDataView data,
    CredentialInfo* out) {
  out->type = CredentialType::CREDENTIAL_TYPE_PASSWORD;
  return data.ReadId(&out->id) && data.ReadName(&out->name) &&
         data.ReadIcon(&out->icon) && data.ReadPassword(&out->password);
}

// static
bool StructTraits<mojom::FederatedCredentialDataView, CredentialInfo>::Read(
    mojom::FederatedCredentialDataView data,
    CredentialInfo* out) {
  out->type = CredentialType::CREDENTIAL_TYPE_FEDERATED;
  return data.ReadId(&out->id) && data.ReadName(&out->name) &&
         data.ReadIcon(&out->icon) && data.ReadFederation(&out->federation);
}

}  // namespace mojo
