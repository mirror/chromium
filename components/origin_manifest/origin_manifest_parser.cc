// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/origin_manifest/origin_manifest_parser.h"

#include "base/json/json_reader.h"
#include "base/strings/string_util.h"

namespace origin_manifest {

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
      /*
              // strict-transport-security
              case TLS_DIR:
                base::DictionaryValue* tls_dict;
                if (it->second.GetAsDictionary(&tls_dict)) {
                  om->tls =
         OriginManifestParser::ParseStrictTransportSecurity(tls_dict);
                }
                break;
              // referrer policy
              case REFERRER_DIR:
                base::DictionaryValue* referrer_dict;
                if (it->second.GetAsDictionary(&referrer_dict)) {
                  om->referrer =
         OriginManifestParser::ParseReferrerPolicy(referrer_dict);
                }
                break;
      */
      // content-security-policy
      case CSP_DIR:
        if (it->second.is_list()) {
          om->csps = OriginManifestParser::ParseContentSecurityPolicies(
              std::move(it->second.GetList()));
        }
        break;
      /*
              // client-hints
              case CLIENT_HINTS_DIR:
                if (it->second.is_list()) {
                  om->clienthints =
         OriginManifestParser::ParseClientHints(std::move(it->second.GetList()));
                }
                break;
      */
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
  /*
    if (base::EqualsCaseInsensitiveASCII(value, "strict-transport-security")) {
      return TLS_DIR;
    } else if (base::EqualsCaseInsensitiveASCII(value, "referrer")) {
      return REFERRER_DIR;
    } else if (base::EqualsCaseInsensitiveASCII(value, "client-hints")) {
      return CLIENT_HINTS_DIR;
    } else
  */
  if (base::EqualsCaseInsensitiveASCII(value, "content-security-policy")) {
    return CSP_DIR;
  } else if (base::EqualsCaseInsensitiveASCII(value, "cors-preflights")) {
    return CORS_PREFLIGHTS_DIR;
  }
  return UNKNOWN_DIR;
}

/*
OriginManifestParser::TLSToken OriginManifestParser::ToTLSToken(std::string
value) { if (base::EqualsCaseInsensitiveASCII(value, "required")) { return
TLS_REQUIRED; } else if (base::EqualsCaseInsensitiveASCII(value,
"public-key()-pins")) { return TLS_PKP; } else if
(base::EqualsCaseInsensitiveASCII(value, "certificate-transparency")) { return
TLS_CT;
  }
  return TLS_UNKNOWN;
}

OriginManifestParser::PKPToken OriginManifestParser::ToPKPToken(std::string
value) { if (base::EqualsCaseInsensitiveASCII(value, "pins")) { return PKP_PINS;
  } else if (base::EqualsCaseInsensitiveASCII(value, "disposition")) {
    return PKP_DISPOSITION;
  } else if (base::EqualsCaseInsensitiveASCII(value, "report-to")) {
    return PKP_REPORT_TO;
  }
  return PKP_UNKNOWN;
}

OriginManifestParser::CTToken OriginManifestParser::ToCTToken(std::string value)
{ if (base::EqualsCaseInsensitiveASCII(value, "disposition")) { return
CT_DISPOSITION; } else if (base::EqualsCaseInsensitiveASCII(value, "report-to"))
{ return CT_REPORT_TO;
  }
  return CT_UNKNOWN;
}

OriginManifestParser::ReferrerPolicyToken
OriginManifestParser::ToReferrerPolicyToken(std::string value) { if
(base::EqualsCaseInsensitiveASCII(value, "policy")) { return REFERRER_POLICY; }
else if (base::EqualsCaseInsensitiveASCII(value, "allow-override")) { return
REFERRER_ALLOW_OVERRIDE;
  }
  return REFERRER_UNKNOWN;
}
*/

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

/*
mojom::StrictTransportSecurityPtr
OriginManifestParser::ParseStrictTransportSecurity(base::DictionaryValue* dict)
{
  // TODO throw some meaningful errors when parsing does not go well
  mojom::StrictTransportSecurityPtr tls = mojom::StrictTransportSecurity::New();

  for (base::detail::dict_iterator_proxy::iterator it =
dict->DictItems().begin(); it != dict->DictItems().end(); ++it) {
    switch(ToTLSToken(it->first)) {
      case TLS_REQUIRED:
        bool is_required;
        if (it->second.GetAsBoolean(&is_required)) {
          tls->required = is_required;
        }
        break;
      case TLS_PKP:
        base::DictionaryValue* pkp_dict;
        if (it->second.GetAsDictionary(&pkp_dict)) {
          tls->pkp = OriginManifestParser::ParsePublicKeyPins(pkp_dict);
        }
        break;
      case TLS_CT:
        base::DictionaryValue* ct_dict;
        if (it->second.GetAsDictionary(&ct_dict)) {
          tls->ct = ParseCertificateTransparency(ct_dict);
        }
        break;
      case TLS_UNKNOWN:
        // just ignore. IMHO that's fair game
        break;
    }
  }

  return tls;
}

mojom::PublicKeyPinsPtr
OriginManifestParser::ParsePublicKeyPins(base::DictionaryValue* dict) {
  // TODO throw some meaningful errors when parsing does not go well
  mojom::PublicKeyPinsPtr pkp = mojom::PublicKeyPins::New();

  for (base::detail::dict_iterator_proxy::iterator it =
dict->DictItems().begin(); it != dict->DictItems().end(); ++it) {
    switch(ToPKPToken(it->first)) {
      case PKP_PINS:
        pkp->pins = std::vector<std::string>();
        if (it->second.is_list()) {
          for (std::vector<base::Value>::iterator l_it =
it->second.GetList().begin(); l_it != it->second.GetList().begin(); ++l_it) {
            std::string pin;
            if (l_it->GetAsString(&pin)) {
              pkp->pins.push_back(pin);
            }
          }
        }
        break;
      case PKP_DISPOSITION:
        pkp->disposition =
OriginManifestParser::ParseDisposition(&(it->second)); break; case
PKP_REPORT_TO:
        {
          std::string reportTo;
          if (it->second.GetAsString(&reportTo)) {
            pkp->reportTo = reportTo;
          }
        }
        break;
      case PKP_UNKNOWN:
        // just ignore. IMHO that's fair game
        break;
    }
  }

  return pkp;
}

mojom::CertificateTransparencyPtr
OriginManifestParser::ParseCertificateTransparency(base::DictionaryValue* dict)
{
  // TODO throw some meaningful errors when parsing does not go well
  mojom::CertificateTransparencyPtr ct = mojom::CertificateTransparency::New();

  for (base::detail::dict_iterator_proxy::iterator it =
dict->DictItems().begin(); it != dict->DictItems().end(); ++it) {
    switch(ToCTToken(it->first)) {
      case CT_DISPOSITION:
        ct->disposition = ParseDisposition(&(it->second));
        break;
      case CT_REPORT_TO:
        {
          std::string reportTo;
          if (it->second.GetAsString(&reportTo)) {
            ct->reportTo = reportTo;
          }
        }
        break;
      case CT_UNKNOWN:
        // just ignore. IMHO that's fair game
        break;
    }
  }

  return ct;
}

mojom::ReferrerPolicyPtr
OriginManifestParser::ParseReferrerPolicy(base::DictionaryValue* dict) {
  // TODO throw some meaningful errors when parsing does not go well
  mojom::ReferrerPolicyPtr referrer = mojom::ReferrerPolicy::New();

  for (base::detail::dict_iterator_proxy::iterator it =
dict->DictItems().begin(); it != dict->DictItems().end(); ++it) {
    switch(ToReferrerPolicyToken(it->first)) {
      case REFERRER_POLICY:
        {
          std::string policy;
          if (it->second.GetAsString(&policy)) {
            referrer->policy = policy;
          }
        }
        break;
      case REFERRER_ALLOW_OVERRIDE:
        {
          bool allow_override;
          if (it->second.GetAsBoolean(&allow_override)) {
            referrer->allowOverride = allow_override;
          }
        }
        break;
      case REFERRER_UNKNOWN:
        // just ignore. IMHO that's fair game
        break;
    }
  }

  return referrer;
}
*/

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

/*
mojom::ClientHintsPtr
OriginManifestParser::ParseClientHints(std::vector<base::Value> list) {
  // TODO throw some meaningful errors when parsing does not go well
  mojom::ClientHintsPtr clienthints = mojom::ClientHints::New();

  for (std::vector<base::Value>::iterator it = list.begin(); it != list.end();
++it) { std::string headername; if (it->GetAsString(&headername)) {
      clienthints->headernames.push_back(headername);
    }
  }

  return clienthints;
}
*/

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

}  // namespace origin_manifest
