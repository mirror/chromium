// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/origin_manifest/origin_manifest_parser.h"

#include "base/json/json_reader.h"
#include "base/strings/string_util.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

blink::mojom::OriginManifestPtr OriginManifestParser::Parse(url::Origin origin,
                                                            std::string version,
                                                            std::string json) {
  // TODO throw some meaningful errors when parsing does not go well
  std::unique_ptr<base::DictionaryValue> directives_dict =
      base::DictionaryValue::From(
          base::JSONReader::Read(json, base::JSON_ALLOW_TRAILING_COMMAS));

  if (directives_dict == nullptr)
    return nullptr;

  blink::mojom::OriginManifestPtr om = blink::mojom::OriginManifest::New();
  // TODO(dhausknecht) this should be moved out of here or at least handled
  // differently where to attach the origin and version
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
  if (value == "enforce") {
    return DISPOSITION_ENFORCE;
  } else if (value == "report-only") {
    return DISPOSITION_REPORT_ONLY;
  }
  return DISPOSITION_UNKNOWN;
}

OriginManifestParser::OriginManifestDirectives
OriginManifestParser::ToDirectivesToken(std::string value) {
  if (value == "content-security-policy") {
    return CSP_DIR;
  } else if (value == "cors-preflights") {
    return CORS_PREFLIGHTS_DIR;
  }
  return UNKNOWN_DIR;
}

OriginManifestParser::CSPToken OriginManifestParser::ToCSPToken(
    std::string value) {
  if (value == "policy") {
    return CSP_POLICY;
  } else if (value == "disposition") {
    return CSP_DISPOSITION;
  } else if (value == "allow-override") {
    return CSP_ALLOW_OVERRIDE;
  }
  return CSP_UNKNOWN;
}

OriginManifestParser::CORSPreflightsToken
OriginManifestParser::ToCORSPreflightsToken(std::string value) {
  if (value == "no-credentials") {
    return CORS_NO_CREDENTIALS;
  } else if (value == "unsafe-include-credentials") {
    return CORS_UNSAFE_INCLUDE_CREDENTIALS;
  }
  return CORS_UNKNOWN;
}

blink::WebContentSecurityPolicyType OriginManifestParser::ParseDisposition(
    base::Value* value) {
  std::string disposition;
  if (value->GetAsString(&disposition)) {
    switch (ToDispositionToken(disposition)) {
      case DISPOSITION_ENFORCE:
      case DISPOSITION_UNKNOWN:
        return blink::WebContentSecurityPolicyType::
            kWebContentSecurityPolicyTypeEnforce;
      case DISPOSITION_REPORT_ONLY:
        return blink::WebContentSecurityPolicyType::
            kWebContentSecurityPolicyTypeReport;
    }
  }
  return blink::WebContentSecurityPolicyType::
      kWebContentSecurityPolicyTypeEnforce;
}

std::vector<blink::mojom::ContentSecurityPolicyPtr>
OriginManifestParser::ParseContentSecurityPolicies(
    std::vector<base::Value> list) {
  // TODO throw some meaningful errors when parsing does not go well
  std::vector<blink::mojom::ContentSecurityPolicyPtr> csps;

  for (std::vector<base::Value>::iterator l_it = list.begin();
       l_it != list.end(); ++l_it) {
    base::DictionaryValue* dict;
    if (l_it->GetAsDictionary(&dict)) {
      blink::mojom::ContentSecurityPolicyPtr csp =
          blink::mojom::ContentSecurityPolicy::New();
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
      csps.push_back(std::move(csp));
    }
  }

  return csps;
}

blink::mojom::CORSOriginsPtr OriginManifestParser::ParseCORSOrigins(
    base::Value* value) {
  blink::mojom::CORSOriginsPtr corsorigins = blink::mojom::CORSOrigins::New();
  // ["a.com", "b.com"]
  if (value->is_list()) {
    for (auto const& elem : value->GetList()) {
      std::string origin_str;
      if (elem.GetAsString(&origin_str)) {
        if (origin_str == "*") {
          corsorigins->allowAll = true;
        } else {
          GURL url(origin_str);
          if (url.is_valid())
            corsorigins->origins.push_back(url::Origin(url));
        }
      }
    }
    // "*" or "a.com"
  } else if (value->is_string()) {
    std::string origin_str;
    if (value->GetAsString(&origin_str)) {
      if (origin_str == "*") {
        corsorigins->allowAll = true;
      } else {
        GURL url(origin_str);
        if (url.is_valid())
          corsorigins->origins.push_back(url::Origin(url));
      }
    }
  } else {
    // just ignore. IMHO that's fair game
  }
  return corsorigins;
}

blink::mojom::CORSPreflightPtr OriginManifestParser::ParseCORSPreflight(
    base::DictionaryValue* dict) {
  // TODO throw some meaningful errors when parsing does not go well
  blink::mojom::CORSPreflightPtr corspreflights =
      blink::mojom::CORSPreflight::New();

  for (base::detail::dict_iterator_proxy::iterator it =
           dict->DictItems().begin();
       it != dict->DictItems().end(); ++it) {
    switch (ToCORSPreflightsToken(it->first)) {
      case CORS_NO_CREDENTIALS: {
        base::DictionaryValue* creds_dict;
        if (it->second.GetAsDictionary(&creds_dict)) {
          for (base::detail::dict_iterator_proxy::iterator d_it =
                   creds_dict->DictItems().begin();
               d_it != creds_dict->DictItems().end(); ++d_it) {
            if (d_it->first == "origins") {
              corspreflights->nocredentials = ParseCORSOrigins(&(d_it->second));
            }
            // unknown? Just ignore. IMHO that's fair game
          }
        }
      }
      // Should I go nuts when this happens?
      break;
      case CORS_UNSAFE_INCLUDE_CREDENTIALS: {
        base::DictionaryValue* creds_dict;
        if (it->second.GetAsDictionary(&creds_dict)) {
          for (base::detail::dict_iterator_proxy::iterator d_it =
                   creds_dict->DictItems().begin();
               d_it != creds_dict->DictItems().end(); ++d_it) {
            if (d_it->first == "origins") {
              corspreflights->withcredentials =
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
