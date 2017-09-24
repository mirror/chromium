// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/origin_manifest/origin_manifest_parser.h"

#include "base/json/json_reader.h"
#include "base/strings/string_util.h"

namespace content {

mojom::OriginManifestPtr OriginManifestParser::Parse(std::string origin,
                                                     std::string version,
                                                     std::string json) {
  // TODO throw some meaningful errors when parsing does not go well
  std::unique_ptr<base::DictionaryValue> directives_dict =
      base::DictionaryValue::From(
          base::JSONReader::Read(json, base::JSON_ALLOW_TRAILING_COMMAS));

  if (directives_dict == nullptr)
    return nullptr;

  mojom::OriginManifestPtr om = mojom::OriginManifest::New();
  om->origin = origin;
  om->version = version;
  om->json = json;

  for (base::detail::dict_iterator_proxy::iterator it =
           directives_dict->DictItems().begin();
       it != directives_dict->DictItems().end(); ++it) {
    switch (OriginManifestParser::ToDirectivesToken(it->first)) {
      // content-security-policy
      case CSP_DIR:
        if (it->second.is_list()) {
          om->csps = OriginManifestParser::ParseContentSecurityPolicies(
              std::move(it->second.GetList()));
        }
        break;
      // cors-preflights
      case CORS_PREFLIGHTS_DIR:
        base::DictionaryValue* cors_dict;
        if (it->second.GetAsDictionary(&cors_dict)) {
          om->corspreflights =
              OriginManifestParser::ParseCORSPreflight(cors_dict);
        }
        break;
      case UNKNOWN_DIR:
        // just ignore. IMHO that's fair game
        break;
    }
  }

  return om;
}

OriginManifestParser::DispositionToken OriginManifestParser::ToDispositionToken(
    std::string value) {
  if (base::EqualsCaseInsensitiveASCII(value, "enforce")) {
    return DISPOSITION_ENFORCE;
  } else if (base::EqualsCaseInsensitiveASCII(value, "report-only")) {
    return DISPOSITION_REPORT_ONLY;
  }
  return DISPOSITION_UNKNOWN;
}

OriginManifestParser::OriginManifestDirectives
OriginManifestParser::ToDirectivesToken(std::string value) {
  if (base::EqualsCaseInsensitiveASCII(value, "content-security-policy")) {
    return CSP_DIR;
  } else if (base::EqualsCaseInsensitiveASCII(value, "cors-preflights")) {
    return CORS_PREFLIGHTS_DIR;
  }
  return UNKNOWN_DIR;
}

OriginManifestParser::CSPToken OriginManifestParser::ToCSPToken(
    std::string value) {
  if (base::EqualsCaseInsensitiveASCII(value, "policy")) {
    return CSP_POLICY;
  } else if (base::EqualsCaseInsensitiveASCII(value, "disposition")) {
    return CSP_DISPOSITION;
  } else if (base::EqualsCaseInsensitiveASCII(value, "allow-override")) {
    return CSP_ALLOW_OVERRIDE;
  }
  return CSP_UNKNOWN;
}

OriginManifestParser::CORSPreflightsToken
OriginManifestParser::ToCORSPreflightsToken(std::string value) {
  if (base::EqualsCaseInsensitiveASCII(value, "no-credentials")) {
    return CORS_NO_CREDENTIALS;
  } else if (base::EqualsCaseInsensitiveASCII(value,
                                              "unsafe-include-credentials")) {
    return CORS_UNSAFE_INCLUDE_CREDENTIALS;
  }
  return CORS_UNKNOWN;
}

mojom::Disposition OriginManifestParser::ParseDisposition(base::Value* value) {
  std::string disposition;
  if (value->GetAsString(&disposition)) {
    switch (ToDispositionToken(disposition)) {
      case DISPOSITION_ENFORCE:
      case DISPOSITION_UNKNOWN:
        return mojom::Disposition::ENFORCE;
      case DISPOSITION_REPORT_ONLY:
        return mojom::Disposition::REPORT_ONLY;
    }
  }
  return mojom::Disposition::ENFORCE;
}

std::vector<mojom::ContentSecurityPolicyPtr>
OriginManifestParser::ParseContentSecurityPolicies(
    std::vector<base::Value> list) {
  // TODO throw some meaningful errors when parsing does not go well
  std::vector<mojom::ContentSecurityPolicyPtr> csps =
      std::vector<mojom::ContentSecurityPolicyPtr>();

  for (std::vector<base::Value>::iterator l_it = list.begin();
       l_it != list.end(); ++l_it) {
    mojom::ContentSecurityPolicyPtr csp = mojom::ContentSecurityPolicy::New();
    base::DictionaryValue* dict;
    if (l_it->GetAsDictionary(&dict)) {
      for (base::detail::dict_iterator_proxy::iterator d_it =
               dict->DictItems().begin();
           d_it != dict->DictItems().end(); ++d_it) {
        switch (ToCSPToken(d_it->first)) {
          case CSP_POLICY: {
            std::string policy;
            if (d_it->second.GetAsString(&policy)) {
              csp->policy = policy;
            }
          } break;
          case CSP_DISPOSITION:
            csp->disposition = ParseDisposition(&(d_it->second));
            break;
          case CSP_ALLOW_OVERRIDE: {
            bool allow_override;
            if (d_it->second.GetAsBoolean(&allow_override)) {
              csp->allowOverride = allow_override;
            }
          } break;
          case CSP_UNKNOWN:
            // just ignore. IMHO that's fair game
            break;
        }
      }
    }
    csps.push_back(std::move(csp));
  }

  return csps;
}

std::vector<std::string> OriginManifestParser::ParseCORSOrigins(
    base::Value* value) {
  std::vector<std::string> origins;
  // ["a.com", "b.com"]
  if (value->is_list()) {
    for (auto const& elem : value->GetList()) {
      std::string origin;
      if (elem.GetAsString(&origin)) {
        origins.push_back(origin);
      }
    }
    // "*" or "a.com"
  } else if (value->is_string()) {
    std::string origin;
    if (value->GetAsString(&origin)) {
      origins.push_back(origin);
    }
  } else {
    // zet schould not häppen. Nein nein
  }
  return origins;
}

mojom::CORSPreflightPtr OriginManifestParser::ParseCORSPreflight(
    base::DictionaryValue* dict) {
  // TODO throw some meaningful errors when parsing does not go well
  mojom::CORSPreflightPtr corspreflights = mojom::CORSPreflight::New();

  for (base::detail::dict_iterator_proxy::iterator it =
           dict->DictItems().begin();
       it != dict->DictItems().end(); ++it) {
    switch (ToCORSPreflightsToken(it->first)) {
      case CORS_NO_CREDENTIALS: {
        corspreflights->nocredentials = mojom::CORSNoCredentials::New();
        base::DictionaryValue* no_creds_dict;
        if (it->second.GetAsDictionary(&no_creds_dict)) {
          for (base::detail::dict_iterator_proxy::iterator d_it =
                   no_creds_dict->DictItems().begin();
               d_it != no_creds_dict->DictItems().end(); ++d_it) {
            if (base::EqualsCaseInsensitiveASCII(d_it->first, "origins")) {
              corspreflights->nocredentials->origins =
                  ParseCORSOrigins(&(d_it->second));
            }
            // unknown? Just ignore. IMHO that's fair game
          }
        }
      }
      // Should I go nuts when this happens?
      break;
      case CORS_UNSAFE_INCLUDE_CREDENTIALS: {
        corspreflights->withcredentials = mojom::CORSWithCredentials::New();
        base::DictionaryValue* with_creds_dict;
        if (it->second.GetAsDictionary(&with_creds_dict)) {
          for (base::detail::dict_iterator_proxy::iterator d_it =
                   with_creds_dict->DictItems().begin();
               d_it != with_creds_dict->DictItems().end(); ++d_it) {
            if (base::EqualsCaseInsensitiveASCII(d_it->first, "origins")) {
              corspreflights->withcredentials->origins =
                  ParseCORSOrigins(&(d_it->second));
            }
            // unknown? Just ignore. IMHO that's fair game
          }
        }
      }
      // Should I go nuts when this happens?
      break;
      case CORS_UNKNOWN:
        // just ignore. IMHO that's fair game
        break;
    }
  }

  return corspreflights;
}

}  // namespace content
