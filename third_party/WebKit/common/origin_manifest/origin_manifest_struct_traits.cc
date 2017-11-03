// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/common/origin_manifest/origin_manifest_struct_traits.h"

#include "base/strings/string_piece.h"

namespace mojo {

// static
blink::OriginManifest::ContentSecurityPolicyType
StructTraits<blink::mojom::OriginManifestContentSecurityPolicyDataView,
             blink::OriginManifest::ContentSecurityPolicy>::
    disposition(const blink::OriginManifest::ContentSecurityPolicy& csp) {
  return csp.disposition;
}

// static
bool StructTraits<blink::mojom::OriginManifestContentSecurityPolicyDataView,
                  blink::OriginManifest::ContentSecurityPolicy>::
    Read(blink::mojom::OriginManifestContentSecurityPolicyDataView data,
         blink::OriginManifest::ContentSecurityPolicy* output) {
  *output = blink::OriginManifest::ContentSecurityPolicy();

  base::StringPiece policy;
  blink::OriginManifest::ContentSecurityPolicyType disposition;
  if (!data.ReadPolicy(&policy) || !data.ReadDisposition(&disposition))
    return false;

  base::internal::CopyToString(policy, &(output->policy));
  output->disposition = disposition;

  return true;
}

// static
const std::vector<blink::OriginManifest::ContentSecurityPolicy>& StructTraits<
    blink::mojom::OriginManifestDataView,
    blink::OriginManifest>::csp_baseline(const blink::OriginManifest& om) {
  return std::move(om.GetContentSecurityPolicies(
      blink::OriginManifest::FallbackDisposition::kBaselineOnly));
}

// static
const std::vector<blink::OriginManifest::ContentSecurityPolicy>& StructTraits<
    blink::mojom::OriginManifestDataView,
    blink::OriginManifest>::csp_fallback(const blink::OriginManifest& om) {
  auto csp_baseline = om.GetContentSecurityPolicies(
      blink::OriginManifest::FallbackDisposition::kBaselineOnly);
  auto csp_all = om.GetContentSecurityPolicies(
      blink::OriginManifest::FallbackDisposition::kIncludeFallbacks);

  for (const auto& csp : csp_baseline) {
    // TODO(dhausknecht) We should add another filter option
    // FallbackDisposition::kFallbacksOnly
    std::vector<blink::OriginManifest::ContentSecurityPolicy>::iterator it;
    for (it = csp_all.begin(); it != csp_all.end(); ++it) {
      // compare against |csp| and break on hit
      if (it->policy == csp.policy && it->disposition == csp.disposition)
        break;
    }
    // I know erase is kind of expensive, but see the TODO above...
    if (it != csp_all.end())
      csp_all.erase(it);
  }

  return std::move(csp_all);
}

// static
bool StructTraits<blink::mojom::OriginManifestDataView, blink::OriginManifest>::
    Read(blink::mojom::OriginManifestDataView data,
         blink::OriginManifest* output) {
  *output = blink::OriginManifest();

  // TODO read out and set CSPs

  return true;
}

}  // namespace mojo
