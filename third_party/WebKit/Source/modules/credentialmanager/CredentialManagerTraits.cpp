// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/credentialmanager/CredentialManagerTraits.h"

namespace mojo {

// static
credential_manager::mojom::blink::CredentialMediationRequirement
EnumTraits<credential_manager::mojom::blink::CredentialMediationRequirement,
           blink::WebCredentialMediationRequirement>::
    ToMojom(blink::WebCredentialMediationRequirement input) {
  switch (input) {
    case blink::WebCredentialMediationRequirement::kSilent:
      return credential_manager::mojom::blink::CredentialMediationRequirement::
          kSilent;
    case blink::WebCredentialMediationRequirement::kOptional:
      return credential_manager::mojom::blink::CredentialMediationRequirement::
          kOptional;
    case blink::WebCredentialMediationRequirement::kRequired:
      return credential_manager::mojom::blink::CredentialMediationRequirement::
          kRequired;
  }

  NOTREACHED();
  return credential_manager::mojom::blink::CredentialMediationRequirement::
      kOptional;
}

// static
bool EnumTraits<
    credential_manager::mojom::blink::CredentialMediationRequirement,
    blink::WebCredentialMediationRequirement>::
    FromMojom(
        credential_manager::mojom::blink::CredentialMediationRequirement input,
        blink::WebCredentialMediationRequirement* output) {
  switch (input) {
    case credential_manager::mojom::blink::CredentialMediationRequirement::
        kSilent:
      *output = blink::WebCredentialMediationRequirement::kSilent;
      return true;
    case credential_manager::mojom::blink::CredentialMediationRequirement::
        kOptional:
      *output = blink::WebCredentialMediationRequirement::kOptional;
      return true;
    case credential_manager::mojom::blink::CredentialMediationRequirement::
        kRequired:
      *output = blink::WebCredentialMediationRequirement::kRequired;
      return true;
  }

  NOTREACHED();
  return false;
}

// static
credential_manager::mojom::blink::CredentialManagerError EnumTraits<
    credential_manager::mojom::blink::CredentialManagerError,
    blink::WebCredentialManagerError>::ToMojom(blink::WebCredentialManagerError
                                                   input) {
  switch (input) {
    case blink::kWebCredentialManagerNoError:
      return credential_manager::mojom::blink::CredentialManagerError::SUCCESS;
    case blink::kWebCredentialManagerDisabledError:
      return credential_manager::mojom::blink::CredentialManagerError::DISABLED;
    case blink::kWebCredentialManagerPendingRequestError:
      return credential_manager::mojom::blink::CredentialManagerError::
          PENDINGREQUEST;
    case blink::kWebCredentialManagerPasswordStoreUnavailableError:
      return credential_manager::mojom::blink::CredentialManagerError::
          PASSWORDSTOREUNAVAILABLE;
    case blink::kWebCredentialManagerUnknownError:
      return credential_manager::mojom::blink::CredentialManagerError::UNKNOWN;
  }

  NOTREACHED();
  return credential_manager::mojom::blink::CredentialManagerError::UNKNOWN;
}

// static
bool EnumTraits<credential_manager::mojom::blink::CredentialManagerError,
                blink::WebCredentialManagerError>::
    FromMojom(credential_manager::mojom::blink::CredentialManagerError input,
              blink::WebCredentialManagerError* output) {
  switch (input) {
    case credential_manager::mojom::blink::CredentialManagerError::SUCCESS:
      *output = blink::kWebCredentialManagerNoError;
      return true;
    case credential_manager::mojom::blink::CredentialManagerError::DISABLED:
      *output = blink::kWebCredentialManagerDisabledError;
      return true;
    case credential_manager::mojom::blink::CredentialManagerError::
        PENDINGREQUEST:
      *output = blink::kWebCredentialManagerPendingRequestError;
      return true;
    case credential_manager::mojom::blink::CredentialManagerError::
        PASSWORDSTOREUNAVAILABLE:
      *output = blink::kWebCredentialManagerPasswordStoreUnavailableError;
      return true;
    case credential_manager::mojom::blink::CredentialManagerError::UNKNOWN:
      *output = blink::kWebCredentialManagerUnknownError;
      return true;
  }

  NOTREACHED();
  return false;
}

// static
bool UnionTraits<credential_manager::mojom::blink::Credential::DataView,
                 blink::Persistent<blink::Credential>>::
    Read(credential_manager::mojom::blink::Credential::DataView data,
         blink::Persistent<blink::Credential>* out) {
  switch (data.tag()) {
    case credential_manager::mojom::blink::Credential::Tag::
        PASSWORD_CREDENTIAL: {
      blink::Persistent<blink::PasswordCredential> subtype_out;
      if (!data.ReadPasswordCredential(&subtype_out))
        return false;
      *out = subtype_out;
      return true;
    }
    case credential_manager::mojom::blink::Credential::Tag::
        FEDERATED_CREDENTIAL: {
      blink::Persistent<blink::FederatedCredential> subtype_out;
      if (!data.ReadFederatedCredential(&subtype_out))
        return false;
      *out = subtype_out;
      return true;
    }
  }
  NOTREACHED();
  return false;
}

// static
bool StructTraits<
    credential_manager::mojom::blink::PasswordCredential::DataView,
    blink::Persistent<blink::PasswordCredential>>::
    Read(credential_manager::mojom::blink::PasswordCredential::DataView data,
         blink::Persistent<blink::PasswordCredential>* out) {
  WTF::String id, name, password;
  blink::KURL icon;
  if (!data.ReadId(&id) || !data.ReadName(&name) || !data.ReadIcon(&icon) ||
      !data.ReadPassword(&password)) {
    return false;
  }
  *out = blink::PasswordCredential::Create(id, password, name, icon);
  return true;
}

// static
bool StructTraits<
    credential_manager::mojom::blink::FederatedCredential::DataView,
    blink::Persistent<blink::FederatedCredential>>::
    Read(credential_manager::mojom::blink::FederatedCredential::DataView data,
         blink::Persistent<blink::FederatedCredential>* out) {
  WTF::String id, name;
  blink::KURL icon;
  RefPtr<blink::SecurityOrigin> federation;
  if (!data.ReadId(&id) || !data.ReadName(&name) || !data.ReadIcon(&icon) ||
      !data.ReadFederation(&federation)) {
    return false;
  }
  *out = blink::FederatedCredential::Create(id, federation, name, icon);
  return true;
}

}  // namespace mojo
