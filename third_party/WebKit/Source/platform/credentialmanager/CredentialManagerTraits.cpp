// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/credentialmanager/CredentialManagerTraits.h"

#include "url/mojo/origin_struct_traits.h"
#include "url/mojo/url_gurl_struct_traits.h"

namespace mojo {

WTF::String GetEmptyIfNull(const WTF::String& s) {
  if (s.IsNull())
    return g_empty_string;
  return s;
}

// static
password_manager::mojom::blink::CredentialMediationRequirement
EnumTraits<password_manager::mojom::blink::CredentialMediationRequirement,
           ::blink::WebCredentialMediationRequirement>::
    ToMojom(::blink::WebCredentialMediationRequirement input) {
  switch (input) {
    case ::blink::WebCredentialMediationRequirement::kSilent:
      return password_manager::mojom::blink::CredentialMediationRequirement::
          kSilent;
    case ::blink::WebCredentialMediationRequirement::kOptional:
      return password_manager::mojom::blink::CredentialMediationRequirement::
          kOptional;
    case ::blink::WebCredentialMediationRequirement::kRequired:
      return password_manager::mojom::blink::CredentialMediationRequirement::
          kRequired;
  }

  NOTREACHED();
  return password_manager::mojom::blink::CredentialMediationRequirement::
      kOptional;
}

// static
bool EnumTraits<password_manager::mojom::blink::CredentialMediationRequirement,
                ::blink::WebCredentialMediationRequirement>::
    FromMojom(
        password_manager::mojom::blink::CredentialMediationRequirement input,
        ::blink::WebCredentialMediationRequirement* output) {
  switch (input) {
    case password_manager::mojom::blink::CredentialMediationRequirement::
        kSilent:
      *output = ::blink::WebCredentialMediationRequirement::kSilent;
      return true;
    case password_manager::mojom::blink::CredentialMediationRequirement::
        kOptional:
      *output = ::blink::WebCredentialMediationRequirement::kOptional;
      return true;
    case password_manager::mojom::blink::CredentialMediationRequirement::
        kRequired:
      *output = ::blink::WebCredentialMediationRequirement::kRequired;
      return true;
  }

  NOTREACHED();
  return false;
}

// static
password_manager::mojom::blink::CredentialManagerError
EnumTraits<password_manager::mojom::blink::CredentialManagerError,
           ::blink::WebCredentialManagerError>::
    ToMojom(::blink::WebCredentialManagerError input) {
  switch (input) {
    case ::blink::kWebCredentialManagerNoError:
      return password_manager::mojom::blink::CredentialManagerError::SUCCESS;
    case ::blink::kWebCredentialManagerDisabledError:
      return password_manager::mojom::blink::CredentialManagerError::DISABLED;
    case ::blink::kWebCredentialManagerPendingRequestError:
      return password_manager::mojom::blink::CredentialManagerError::
          PENDINGREQUEST;
    case ::blink::kWebCredentialManagerPasswordStoreUnavailableError:
      return password_manager::mojom::blink::CredentialManagerError::
          PASSWORDSTOREUNAVAILABLE;
    case ::blink::kWebCredentialManagerUnknownError:
      return password_manager::mojom::blink::CredentialManagerError::UNKNOWN;
  }

  NOTREACHED();
  return password_manager::mojom::blink::CredentialManagerError::UNKNOWN;
}

// static
bool EnumTraits<password_manager::mojom::blink::CredentialManagerError,
                ::blink::WebCredentialManagerError>::
    FromMojom(password_manager::mojom::blink::CredentialManagerError input,
              ::blink::WebCredentialManagerError* output) {
  switch (input) {
    case password_manager::mojom::blink::CredentialManagerError::SUCCESS:
      *output = ::blink::kWebCredentialManagerNoError;
      return true;
    case password_manager::mojom::blink::CredentialManagerError::DISABLED:
      *output = ::blink::kWebCredentialManagerDisabledError;
      return true;
    case password_manager::mojom::blink::CredentialManagerError::PENDINGREQUEST:
      *output = ::blink::kWebCredentialManagerPendingRequestError;
      return true;
    case password_manager::mojom::blink::CredentialManagerError::
        PASSWORDSTOREUNAVAILABLE:
      *output = ::blink::kWebCredentialManagerPasswordStoreUnavailableError;
      return true;
    case password_manager::mojom::blink::CredentialManagerError::UNKNOWN:
      *output = ::blink::kWebCredentialManagerUnknownError;
      return true;
  }

  NOTREACHED();
  return false;
}

using CredentialInfoTraits =
    StructTraits<password_manager::mojom::blink::CredentialInfo::DataView,
                 std::unique_ptr<::blink::WebCredential>>;

// static
password_manager::mojom::blink::CredentialType CredentialInfoTraits::type(
    const std::unique_ptr<::blink::WebCredential>& r) {
  if (r->IsPasswordCredential())
    return password_manager::mojom::blink::CredentialType::PASSWORD;
  if (r->IsFederatedCredential())
    return password_manager::mojom::blink::CredentialType::FEDERATED;
  return password_manager::mojom::blink::CredentialType::EMPTY;
}

// static
WTF::String CredentialInfoTraits::id(
    const std::unique_ptr<::blink::WebCredential>& r) {
  return GetEmptyIfNull(r->Id());
}

// static
WTF::String CredentialInfoTraits::name(
    const std::unique_ptr<::blink::WebCredential>& r) {
  if (r->IsPasswordCredential()) {
    return GetEmptyIfNull(
        static_cast<const ::blink::WebPasswordCredential&>(*r).Name());
  }
  if (r->IsFederatedCredential()) {
    return GetEmptyIfNull(
        static_cast<const ::blink::WebFederatedCredential&>(*r).Name());
  }
  return g_empty_string;
}

// static
::blink::KURL CredentialInfoTraits::icon(
    const std::unique_ptr<::blink::WebCredential>& r) {
  if (r->IsPasswordCredential())
    return static_cast<const ::blink::WebPasswordCredential&>(*r).IconURL();
  if (r->IsFederatedCredential())
    return static_cast<const ::blink::WebFederatedCredential&>(*r).IconURL();
  return ::blink::KURL();
}

// static
WTF::String CredentialInfoTraits::password(
    const std::unique_ptr<::blink::WebCredential>& r) {
  if (r->IsPasswordCredential()) {
    return GetEmptyIfNull(
        static_cast<const ::blink::WebPasswordCredential&>(*r).Password());
  }
  return g_empty_string;
}

// static
RefPtr<::blink::SecurityOrigin> CredentialInfoTraits::federation(
    const std::unique_ptr<::blink::WebCredential>& r) {
  if (r->IsFederatedCredential()) {
    return static_cast<const ::blink::WebFederatedCredential&>(*r).Provider();
  }
  return ::blink::SecurityOrigin::CreateUnique();
}

// static
bool StructTraits<password_manager::mojom::blink::CredentialInfo::DataView,
                  std::unique_ptr<::blink::WebCredential>>::
    Read(password_manager::mojom::blink::CredentialInfo::DataView data,
         std::unique_ptr<::blink::WebCredential>* out) {
  WTF::String id, name, password;
  ::blink::KURL icon;
  RefPtr<::blink::SecurityOrigin> provider;
  DCHECK(out);
  switch (data.type()) {
    case password_manager::mojom::blink::CredentialType::FEDERATED:
      if (!data.ReadId(&id) || !data.ReadName(&name) || !data.ReadIcon(&icon) ||
          !data.ReadFederation(&provider)) {
        return false;
      }
      *out = base::MakeUnique<blink::WebFederatedCredential>(id, provider, name,
                                                             icon);
      return true;
    case password_manager::mojom::blink::CredentialType::PASSWORD:
      if (!data.ReadId(&id) || !data.ReadName(&name) || !data.ReadIcon(&icon) ||
          !data.ReadPassword(&password)) {
        return false;
      }
      *out = base::MakeUnique<blink::WebPasswordCredential>(id, password, name,
                                                            icon);
      return true;
    case password_manager::mojom::blink::CredentialType::EMPTY:
      DCHECK(!out->get());
      return true;
  }

  NOTREACHED();
  return false;
}

}  // namespace mojo
