// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_COMMON_ORIGIN_MANIFEST_ORIGIN_MANIFEST_H_
#define THIRD_PARTY_WEBKIT_COMMON_ORIGIN_MANIFEST_ORIGIN_MANIFEST_H_

#include "third_party/WebKit/common/content_security_policy.h"

#include <string>
#include <vector>

#include "url/origin.h"

namespace blink {

struct ContentSecurityPolicy {
  std::string policy;
  ContentSecurityPolicyType disposition;
};

struct CORSOrigins {
  // set to true when |origins| contains star
  bool allow_all;
  std::vector<url::Origin> origins;
};

class OriginManifest {
 public:
  void AddContentSecurityPolicy(std::string policy,
                                ContentSecurityPolicyType disposition,
                                bool is_fallback);

  // Returns all CSPs that should be enforced. That is when |has_csp_header| is
  // true only baseline CSPs are returned. All CSPs otherwise.
  const std::vector<ContentSecurityPolicy> GetContentSecurityPolicies(
      bool has_csp_header);

 private:
  std::vector<ContentSecurityPolicy> baseline_csps;
  std::vector<ContentSecurityPolicy> fallback_csps;
};

}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_COMMON_ORIGIN_MANIFEST_ORIGIN_MANIFEST_H_
