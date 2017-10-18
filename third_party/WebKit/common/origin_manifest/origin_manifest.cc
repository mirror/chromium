// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/common/origin_manifest/origin_manifest.h"

namespace blink {

void OriginManifest::AddContentSecurityPolicy(
    std::string policy,
    ContentSecurityPolicyType disposition,
    bool is_fallback) {
  ContentSecurityPolicy csp = {policy, disposition};
  if (is_fallback) {
    fallback_csps.push_back(csp);
  } else {
    baseline_csps.push_back(csp);
  }
}

const std::vector<ContentSecurityPolicy>
OriginManifest::GetContentSecurityPolicies(bool has_csp_header) {
  std::vector<ContentSecurityPolicy> result;
  for (auto csp : baseline_csps) {
    result.push_back(csp);
  }
  if (has_csp_header) {
    for (auto csp : fallback_csps) {
      result.push_back(csp);
    }
  }
  return result;
}

}  // namespace blink
