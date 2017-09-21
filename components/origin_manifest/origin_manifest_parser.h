// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_PARSER_H_
#define COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_PARSER_H_

#include <string>

#include "base/values.h"
#include "components/origin_manifest/interfaces/origin_manifest.mojom.h"

namespace base {
class DictionaryValue;
}

namespace origin_manifest {

class OriginManifestParser {
 public:
  static mojom::OriginManifestPtr Parse(std::string origin,
                                        std::string version,
                                        std::string json);

 private:
  enum DispositionToken {
    DISPOSITION_ENFORCE,
    DISPOSITION_REPORT_ONLY,
    DISPOSITION_UNKNOWN
  };

  enum OriginManifestDirectives {
    // TLS_DIR,
    // REFERRER_DIR,
    CSP_DIR,
    // CLIENT_HINTS_DIR,
    CORS_PREFLIGHTS_DIR,
    UNKNOWN_DIR
  };

  // enum TLSToken {TLS_REQUIRED, TLS_PKP, TLS_CT, TLS_UNKNOWN};

  // enum PKPToken {PKP_PINS, PKP_DISPOSITION, PKP_REPORT_TO, PKP_UNKNOWN};

  // enum CTToken {CT_DISPOSITION, CT_REPORT_TO, CT_UNKNOWN};

  // enum ReferrerPolicyToken {REFERRER_POLICY, REFERRER_ALLOW_OVERRIDE,
  // REFERRER_UNKNOWN};

  enum CSPToken {
    CSP_POLICY,
    CSP_DISPOSITION,
    CSP_ALLOW_OVERRIDE,
    CSP_UNKNOWN
  };

  enum CORSPreflightsToken {
    CORS_NO_CREDENTIALS,
    CORS_UNSAFE_INCLUDE_CREDENTIALS,
    CORS_UNKNOWN
  };

  OriginManifestParser(){};
  ~OriginManifestParser(){};

  static DispositionToken ToDispositionToken(std::string value);
  static OriginManifestDirectives ToDirectivesToken(std::string value);
  // static TLSToken ToTLSToken(std::string value);
  // static PKPToken ToPKPToken(std::string value);
  // static CTToken ToCTToken(std::string value);
  // static ReferrerPolicyToken ToReferrerPolicyToken(std::string value);
  static CSPToken ToCSPToken(std::string value);
  static CORSPreflightsToken ToCORSPreflightsToken(std::string value);

  static mojom::Disposition ParseDisposition(base::Value* value);
  // static mojom::StrictTransportSecurityPtr
  // ParseStrictTransportSecurity(base::DictionaryValue* dict);  static
  // mojom::PublicKeyPinsPtr ParsePublicKeyPins(base::DictionaryValue* dict);
  // static mojom::CertificateTransparencyPtr
  // ParseCertificateTransparency(base::DictionaryValue* dict);  static
  // mojom::ReferrerPolicyPtr ParseReferrerPolicy(base::DictionaryValue* dict);
  static std::vector<mojom::ContentSecurityPolicyPtr>
  ParseContentSecurityPolicies(std::vector<base::Value> list);
  // static mojom::ClientHintsPtr ParseClientHints(std::vector<base::Value>
  // list);
  static mojom::CORSPreflightPtr ParseCORSPreflight(
      base::DictionaryValue* dict);

  static std::vector<std::string> ParseCORSOrigins(base::Value* value);

  DISALLOW_COPY_AND_ASSIGN(OriginManifestParser);
};

}  // namespace origin_manifest

#endif  // COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_PARSER_H_
