// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_PARSER_H_
#define CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_PARSER_H_

#include <string>

#include "base/values.h"
#include "content/common/content_export.h"
#include "third_party/WebKit/public/platform/origin_manifest.mojom.h"

namespace base {
class DictionaryValue;
}

namespace url {
class Origin;
}

namespace content {

class CONTENT_EXPORT OriginManifestParser {
 public:
  static blink::mojom::OriginManifestPtr Parse(url::Origin origin,
                                               std::string version,
                                               std::string json);

 private:
  enum DispositionToken {
    DISPOSITION_ENFORCE,
    DISPOSITION_REPORT_ONLY,
    DISPOSITION_UNKNOWN
  };

  enum OriginManifestDirectives { CSP_DIR, CORS_PREFLIGHTS_DIR, UNKNOWN_DIR };

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

  OriginManifestParser() = delete;
  ~OriginManifestParser() = delete;

  static DispositionToken ToDispositionToken(std::string value);
  static OriginManifestDirectives ToDirectivesToken(std::string value);
  static CSPToken ToCSPToken(std::string value);
  static CORSPreflightsToken ToCORSPreflightsToken(std::string value);

  static blink::WebContentSecurityPolicyType ParseDisposition(
      base::Value* value);
  static std::vector<blink::mojom::ContentSecurityPolicyPtr>
  ParseContentSecurityPolicies(std::vector<base::Value> list);
  static blink::mojom::CORSPreflightPtr ParseCORSPreflight(
      base::DictionaryValue* dict);

  static blink::mojom::CORSOriginsPtr ParseCORSOrigins(base::Value* value);

  FRIEND_TEST_ALL_PREFIXES(OriginManifestParserTest, ParseDisposition);
  FRIEND_TEST_ALL_PREFIXES(OriginManifestParserTest,
                           ParseContentSecurityPolicies);
  FRIEND_TEST_ALL_PREFIXES(OriginManifestParserTest, ParseCORSPreflight);
  FRIEND_TEST_ALL_PREFIXES(OriginManifestParserTest, ParseCORSOrigins);

  DISALLOW_COPY_AND_ASSIGN(OriginManifestParser);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_PARSER_H_
