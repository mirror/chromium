// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/cryptotoken_private/cryptotoken_private_api.h"

#include <stddef.h>

#include "base/base64.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "crypto/sha2.h"
#include "extensions/common/error_utils.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace {

const char kGoogleDotCom[] = "google.com";
constexpr const char* kGoogleGstaticAppIds[] = {
    "https://www.gstatic.com/securitykey/origins.json",
    "https://www.gstatic.com/securitykey/a/google.com/origins.json"};

}  // namespace

namespace extensions {
namespace api {

void CryptotokenRegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterListPref(prefs::kSecurityKeyPermitAttestation);
}

CryptotokenPrivateCanOriginAssertAppIdFunction::
    CryptotokenPrivateCanOriginAssertAppIdFunction()
    : chrome_details_(this) {
}

ExtensionFunction::ResponseAction
CryptotokenPrivateCanOriginAssertAppIdFunction::Run() {
  std::unique_ptr<cryptotoken_private::CanOriginAssertAppId::Params> params =
      cryptotoken_private::CanOriginAssertAppId::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  const GURL origin_url(params->security_origin);
  if (!origin_url.is_valid()) {
    return RespondNow(Error(extensions::ErrorUtils::FormatErrorMessage(
        "Security origin * is not a valid URL", params->security_origin)));
  }
  const GURL app_id_url(params->app_id_url);
  if (!app_id_url.is_valid()) {
    return RespondNow(Error(extensions::ErrorUtils::FormatErrorMessage(
        "appId * is not a valid URL", params->app_id_url)));
  }

  if (origin_url == app_id_url) {
    return RespondNow(OneArgument(base::MakeUnique<base::Value>(true)));
  }

  // Fetch the eTLD+1 of both.
  const std::string origin_etldp1 =
      net::registry_controlled_domains::GetDomainAndRegistry(
          origin_url,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  if (origin_etldp1.empty()) {
    return RespondNow(Error(extensions::ErrorUtils::FormatErrorMessage(
        "Could not find an eTLD for origin *", params->security_origin)));
  }
  const std::string app_id_etldp1 =
      net::registry_controlled_domains::GetDomainAndRegistry(
          app_id_url,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  if (app_id_etldp1.empty()) {
    return RespondNow(Error(extensions::ErrorUtils::FormatErrorMessage(
        "Could not find an eTLD for appId *", params->app_id_url)));
  }
  if (origin_etldp1 == app_id_etldp1) {
    return RespondNow(OneArgument(base::MakeUnique<base::Value>(true)));
  }
  // For legacy purposes, allow google.com origins to assert certain
  // gstatic.com appIds.
  // TODO(juanlang): remove when legacy constraints are removed.
  if (origin_etldp1 == kGoogleDotCom) {
    for (const char* id : kGoogleGstaticAppIds) {
      if (params->app_id_url == id)
        return RespondNow(OneArgument(base::MakeUnique<base::Value>(true)));
    }
  }
  return RespondNow(OneArgument(base::MakeUnique<base::Value>(false)));
}

CryptotokenPrivateIsAppIdHashInEnterpriseContextFunction::
    CryptotokenPrivateIsAppIdHashInEnterpriseContextFunction()
    : chrome_details_(this) {}

ExtensionFunction::ResponseAction
CryptotokenPrivateIsAppIdHashInEnterpriseContextFunction::Run() {
  std::unique_ptr<cryptotoken_private::IsAppIdHashInEnterpriseContext::Params>
      params(
          cryptotoken_private::IsAppIdHashInEnterpriseContext::Params::Create(
              *args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  std::string app_id_hash_base64 = params->app_id_hash_base64;
  // Pad the provided base64 string with '=' characters because |Base64Decode|
  // demands it.
  app_id_hash_base64.append((4 - (app_id_hash_base64.size() % 4)) % 4, '=');
  std::string app_id_hash;
  if (!base::Base64Decode(app_id_hash_base64, &app_id_hash)) {
    LOG(ERROR)
        << "isAppIdHashInEnterpriseContext: failed to base64 decode argument: "
        << app_id_hash_base64;
    return RespondNow(OneArgument(base::MakeUnique<base::Value>(false)));
  }

  Profile* const profile = Profile::FromBrowserContext(browser_context());
  const PrefService* const prefs = profile->GetPrefs();
  const base::ListValue* const permit_attestation =
      prefs->GetList(prefs::kSecurityKeyPermitAttestation);

  bool result = false;
  for (const auto& i : *permit_attestation) {
    const std::string& s = i.GetString();
    if (s.find("/") == std::string::npos) {
      // No slashes mean that this is a webauthn RP ID, not a U2F AppID.
      continue;
    }

    if (crypto::SHA256HashString(s) == app_id_hash) {
      result = true;
      break;
    }
  }

  return RespondNow(OneArgument(base::MakeUnique<base::Value>(result)));
}

}  // namespace api
}  // namespace extensions
