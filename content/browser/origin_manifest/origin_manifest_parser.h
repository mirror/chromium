// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_PARSER_H_
#define CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_PARSER_H_

#include <string>

#include "base/gtest_prod_util.h"
#include "base/values.h"
#include "content/common/content_export.h"
#include "third_party/WebKit/common/origin_manifest/origin_manifest.h"

namespace base {
class Value;
}

namespace content {

class CONTENT_EXPORT OriginManifestParser {
 public:
  static std::unique_ptr<blink::OriginManifest> Parse(std::string json);

 private:
  enum class DirectiveType {
    kContentSecurityPolicy,
    kUnknown,
  };

  OriginManifestParser() = delete;
  ~OriginManifestParser() = delete;

  static DirectiveType GetDirectiveType(const std::string& str);

  static void ParseContentSecurityPolicy(blink::OriginManifest* const om,
                                         base::Value value);

  static blink::OriginManifest::ContentSecurityPolicyType GetCSPDisposition(
      const std::string& json);
  static blink::ActivationType GetCSPActivationType(const bool value);

  FRIEND_TEST_ALL_PREFIXES(OriginManifestParserTest, GetDirectiveType);
  FRIEND_TEST_ALL_PREFIXES(OriginManifestParserTest,
                           ParseContentSecurityPolicy);
  FRIEND_TEST_ALL_PREFIXES(OriginManifestParserTest, GetCSPDisposition);
  FRIEND_TEST_ALL_PREFIXES(OriginManifestParserTest, GetCSPActivationType);

  DISALLOW_COPY_AND_ASSIGN(OriginManifestParser);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_PARSER_H_
