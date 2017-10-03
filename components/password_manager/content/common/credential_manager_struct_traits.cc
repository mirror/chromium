// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/content/common/credential_manager_struct_traits.h"

#include "url/mojo/origin_struct_traits.h"
#include "url/mojo/url_gurl_struct_traits.h"

using password_manager::CredentialInfo;
using password_manager::CredentialType;
using password_manager::CredentialManagerError;
using password_manager::CredentialMediationRequirement;

namespace mojo {

// static
password_manager::mojom::CredentialType
EnumTraits<password_manager::mojom::CredentialType, CredentialType>::ToMojom(
    CredentialType input) {
  switch (input) {
    case CredentialType::CREDENTIAL_TYPE_EMPTY:
      return password_manager::mojom::CredentialType::EMPTY;
    case CredentialType::CREDENTIAL_TYPE_PASSWORD:
      return password_manager::mojom::CredentialType::PASSWORD;
    case CredentialType::CREDENTIAL_TYPE_FEDERATED:
      return password_manager::mojom::CredentialType::FEDERATED;
  }

  NOTREACHED();
  return password_manager::mojom::CredentialType::EMPTY;
}

// static
bool EnumTraits<password_manager::mojom::CredentialType, CredentialType>::
    FromMojom(password_manager::mojom::CredentialType input,
              CredentialType* output) {
  switch (input) {
    case password_manager::mojom::CredentialType::EMPTY:
      *output = CredentialType::CREDENTIAL_TYPE_EMPTY;
      return true;
    case password_manager::mojom::CredentialType::PASSWORD:
      *output = CredentialType::CREDENTIAL_TYPE_PASSWORD;
      return true;
    case password_manager::mojom::CredentialType::FEDERATED:
      *output = CredentialType::CREDENTIAL_TYPE_FEDERATED;
      return true;
  }

  NOTREACHED();
  return false;
}

// static
password_manager::mojom::CredentialManagerError
EnumTraits<password_manager::mojom::CredentialManagerError,
           CredentialManagerError>::ToMojom(CredentialManagerError input) {
  switch (input) {
    case CredentialManagerError::SUCCESS:
      return password_manager::mojom::CredentialManagerError::SUCCESS;
    case CredentialManagerError::DISABLED:
      return password_manager::mojom::CredentialManagerError::DISABLED;
    case CredentialManagerError::PENDINGREQUEST:
      return password_manager::mojom::CredentialManagerError::PENDINGREQUEST;
    case CredentialManagerError::PASSWORDSTOREUNAVAILABLE:
      return password_manager::mojom::CredentialManagerError::
          PASSWORDSTOREUNAVAILABLE;
    case CredentialManagerError::UNKNOWN:
      return password_manager::mojom::CredentialManagerError::UNKNOWN;
  }

  NOTREACHED();
  return password_manager::mojom::CredentialManagerError::UNKNOWN;
}

// static
bool EnumTraits<password_manager::mojom::CredentialManagerError,
                CredentialManagerError>::
    FromMojom(password_manager::mojom::CredentialManagerError input,
              CredentialManagerError* output) {
  switch (input) {
    case password_manager::mojom::CredentialManagerError::SUCCESS:
      *output = CredentialManagerError::SUCCESS;
      return true;
    case password_manager::mojom::CredentialManagerError::DISABLED:
      *output = CredentialManagerError::DISABLED;
      return true;
    case password_manager::mojom::CredentialManagerError::PENDINGREQUEST:
      *output = CredentialManagerError::PENDINGREQUEST;
      return true;
    case password_manager::mojom::CredentialManagerError::
        PASSWORDSTOREUNAVAILABLE:
      *output = CredentialManagerError::PASSWORDSTOREUNAVAILABLE;
      return true;
    case password_manager::mojom::CredentialManagerError::UNKNOWN:
      *output = CredentialManagerError::UNKNOWN;
      return true;
  }

  NOTREACHED();
  return false;
}

// static
password_manager::mojom::CredentialMediationRequirement EnumTraits<
    password_manager::mojom::CredentialMediationRequirement,
    CredentialMediationRequirement>::ToMojom(CredentialMediationRequirement
                                                 input) {
  switch (input) {
    case CredentialMediationRequirement::kSilent:
      return password_manager::mojom::CredentialMediationRequirement::kSilent;
    case CredentialMediationRequirement::kOptional:
      return password_manager::mojom::CredentialMediationRequirement::kOptional;
    case CredentialMediationRequirement::kRequired:
      return password_manager::mojom::CredentialMediationRequirement::kRequired;
  }

  NOTREACHED();
  return password_manager::mojom::CredentialMediationRequirement::kOptional;
}

// static
bool EnumTraits<password_manager::mojom::CredentialMediationRequirement,
                CredentialMediationRequirement>::
    FromMojom(password_manager::mojom::CredentialMediationRequirement input,
              CredentialMediationRequirement* output) {
  switch (input) {
    case password_manager::mojom::CredentialMediationRequirement::kSilent:
      *output = CredentialMediationRequirement::kSilent;
      return true;
    case password_manager::mojom::CredentialMediationRequirement::kOptional:
      *output = CredentialMediationRequirement::kOptional;
      return true;
    case password_manager::mojom::CredentialMediationRequirement::kRequired:
      *output = CredentialMediationRequirement::kRequired;
      return true;
  }

  NOTREACHED();
  return false;
}

// static
bool StructTraits<
    password_manager::mojom::CredentialInfoDataView,
    CredentialInfo>::Read(password_manager::mojom::CredentialInfoDataView data,
                          CredentialInfo* out) {
  if (data.ReadType(&out->type) && data.ReadId(&out->id) &&
      data.ReadName(&out->name) && data.ReadIcon(&out->icon) &&
      data.ReadPassword(&out->password) &&
      data.ReadFederation(&out->federation))
    return true;

  return false;
}

}  // namespace mojo
