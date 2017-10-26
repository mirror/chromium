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
namespace origin_manifest_parser {

std::unique_ptr<blink::OriginManifest> CONTENT_EXPORT Parse(std::string json);

enum class DirectiveType {
  kContentSecurityPolicy,
  kUnknown,
};

DirectiveType CONTENT_EXPORT GetDirectiveType(const std::string& str);

void CONTENT_EXPORT ParseContentSecurityPolicy(blink::OriginManifest* const om,
                                               base::Value value);

blink::OriginManifest::ContentSecurityPolicyType CONTENT_EXPORT
GetCSPDisposition(const std::string& json);
blink::OriginManifest::ActivationType CONTENT_EXPORT
GetCSPActivationType(const bool value);

}  // namespace origin_manifest_parser
}  // namespace content

#endif  // CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_PARSER_H_
